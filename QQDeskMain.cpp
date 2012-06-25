#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <gdiplus.h>
#include <TlHelp32.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Ole32.lib")

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.Lib")
#pragma comment(lib, "gdi32.Lib")
#pragma comment(lib,"Advapi32.lib")

#define WM_EXITQDESK 10086

HINSTANCE g_hInst;
HBITMAP g_hbmBall = NULL;
RECT g_rcTrayWnd;
RECT g_rcRebar;
HWND g_trayWnd;
HWND g_rebar;
LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM,LPARAM);

HANDLE hTargetProcess;
DWORD  hLibModule = 0;	// base adress of loaded module (==HMODULE);
void*  pLibRemote = 0;	// the address (in the remote process) where szLibPath will be copied to;
char   szLibPath [_MAX_PATH];
HMODULE hKernel32;

TCHAR szWinName[] = TEXT("QDeskWin");

//提升进程访问权限
int enableDebugPriv()
{
	HANDLE hToken;
	LUID sedebugnameValue;
	TOKEN_PRIVILEGES tkp;

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			return false;
	}
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &sedebugnameValue)) {
		CloseHandle(hToken);
		return false;
	}
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = sedebugnameValue;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(tkp), NULL, NULL)) {
		CloseHandle(hToken);
		return false;
	}
	return true;
}

int OpenRemoteThread()
{
	///-----------------------------------------------------------
	//定义线程体的大小
	const DWORD dwThreadSize = 4096;
	DWORD dwWriteBytes;
	//提升进程访问权限
	enableDebugPriv();

	DWORD dwProcessId;
	GetWindowThreadProcessId(g_rebar,&dwProcessId);
	if (dwProcessId == 0) {
		MessageBox(NULL, "The target process have not been found !",
			"Notice", MB_ICONINFORMATION | MB_OK);
		return -1;
	}
	//根据进程ID得到进程句柄
	hTargetProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);

	if (!hTargetProcess) {
		MessageBox(NULL, "Open target process failed !", 
			"Notice", MB_ICONINFORMATION | MB_OK);
		return 0;
	}
	

	if( !GetModuleFileName( g_hInst,szLibPath,_MAX_PATH) )
		return false;
	strcpy( strstr(szLibPath,"QQDeskMain.exe"),"QQDeskRemoteDll.dll" );
	hKernel32 = ::GetModuleHandle("Kernel32");


	// 1. Allocate memory in the remote process for szLibPath
	// 2. Write szLibPath to the allocated memory
	pLibRemote = ::VirtualAllocEx( hTargetProcess, NULL, sizeof(szLibPath), MEM_COMMIT, PAGE_READWRITE );
	if( pLibRemote == NULL )
		return false;
	::WriteProcessMemory(hTargetProcess, pLibRemote, (void*)szLibPath,sizeof(szLibPath),NULL);

	//在宿主进程中创建线程
	HANDLE hRemoteThread = CreateRemoteThread(
		hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,"LoadLibraryA"),
		pLibRemote, 0, &dwWriteBytes);

	if (!hRemoteThread) {
		VirtualFreeEx( hTargetProcess, pLibRemote, dwThreadSize, MEM_DECOMMIT );   
		CloseHandle( hTargetProcess );   
		MessageBox(NULL, "Create remote thread failed !", "Notice",  MB_ICONINFORMATION | MB_OK);
		return 0;
	}
	// 等待加载完毕   
	WaitForSingleObject( hRemoteThread, INFINITE );  
	::GetExitCodeThread( hRemoteThread, &hLibModule ); 
	// 释放目标进程中申请的空间   
	VirtualFreeEx( hTargetProcess, pLibRemote, dwThreadSize, MEM_DECOMMIT );   
	CloseHandle(hRemoteThread);
	///-----------------------------------------------------------
	return true;
}

bool CloseRemoteThread()
{
	///-----------------------------------------------------------
	::VirtualFreeEx( hTargetProcess, pLibRemote, sizeof(szLibPath), MEM_RELEASE );
	if( hLibModule == NULL )
	{
			MessageBox(NULL, "hLibModule NULL !",
			"CloseRemoteThread", MB_ICONINFORMATION | MB_OK);
		return false;
	}

	// Unload ".dll" from the remote process 
	// (via CreateRemoteThread & FreeLibrary)
	HANDLE hRemoteThread = CreateRemoteThread(
		hTargetProcess, NULL, 0, (LPTHREAD_START_ROUTINE) ::GetProcAddress(hKernel32,"FreeLibrary"),
		(void*)hLibModule, 0, NULL);

	if( hRemoteThread == NULL )	// failed to unload
	{
		DWORD errCode = GetLastError();
		char info[256];
		sprintf(info,"hThread lasterror: %d",errCode);
		MessageBox(NULL, info,
			"CloseRemoteThread", MB_ICONINFORMATION | MB_OK);
		return false;
	}

	::WaitForSingleObject( hRemoteThread, INFINITE );
	::GetExitCodeThread( hRemoteThread, &hLibModule );
	::CloseHandle( hRemoteThread );
	CloseHandle( hTargetProcess );
	///-----------------------------------------------------------
	return true;
}

int WINAPI WinMain(HINSTANCE hThisInst,HINSTANCE hPrevInst,LPSTR lpszArgs,int nWinMode)
{
	HRESULT hRes = ::CoInitialize(NULL);
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken; 
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	HWND hwnd;
	MSG msg;
	WNDCLASSEX wcl;
	INITCOMMONCONTROLSEX ic;
	BOOL ret;

	g_hInst = hThisInst;
	ic.dwSize = sizeof(INITCOMMONCONTROLSEX);
	ic.dwICC = 0x00004000;//ICC_STANDARD_CLASSES|ICC_BAR_CLASSES;
	ret = InitCommonControlsEx(&ic);
	wcl.cbSize = sizeof(WNDCLASSEX);
	wcl.hInstance = hThisInst;
	wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = WindowFunc;
	wcl.style = CS_DBLCLKS;//CS_HREDRAW|CS_VREDRAW
	wcl.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wcl.hIconSm = NULL;
	wcl.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcl.lpszMenuName = NULL;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	if(!RegisterClassEx(&wcl)) return 0;
	g_trayWnd = ::FindWindow("Shell_TrayWnd", NULL);
	GetWindowRect(g_trayWnd, &g_rcTrayWnd);
	g_rebar = ::FindWindowEx(g_trayWnd, NULL, "ReBarWindow32", NULL);
	GetWindowRect(g_rebar, &g_rcRebar);
	::SetWindowPos(g_rebar, NULL, g_rcTrayWnd.left+112, 0,   
		g_rcRebar.right - g_rcTrayWnd.left-112, g_rcRebar.bottom - g_rcRebar.top,   
		SWP_NOZORDER|SWP_NOACTIVATE);

	hwnd = CreateWindowEx(
		WS_EX_WINDOWEDGE,
		szWinName,
		TEXT("Window Title"),
		WS_POPUP,
		g_rcTrayWnd.left+56,
		g_rcTrayWnd.top,
		56,
		40,
		HWND_DESKTOP,//HWND_DESKTOP
		NULL,
		hThisInst,
		NULL
		);
	
	//::SetLayeredWindowAttributes( hwnd, RGB(0,0,255), 200, ULW_ALPHA|ULW_COLORKEY);
	ShowWindow(hwnd,SW_SHOW);
	SetWindowPos( hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW );
	UpdateWindow(hwnd);
	
	OpenRemoteThread();
	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	SendMessage(g_rebar,WM_EXITQDESK,0,0);
	CloseRemoteThread();
	GdiplusShutdown(gdiplusToken);
	::CoUninitialize();
	return msg.wParam;
}
LRESULT CALLBACK WindowFunc(HWND hwnd,UINT message,WPARAM
							wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;

	switch(message){
	case WM_CREATE:
		g_hbmBall =  (HBITMAP)LoadImage(0,"back.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE); 
		return 0;
	case WM_SIZE:
		return 0;
	case WM_ERASEBKGND:
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		{
			//BITMAP bm;
			//HDC hdcMem = CreateCompatibleDC(hdc);
			//HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, g_hbmBall);
			Graphics g( hdc );
			Image img(L"QQDesk.ico");
			g.DrawImage( &img, 8, 0,40,40);


			//GetObject(g_hbmBall, sizeof(bm), &bm);
			//BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
			//SelectObject(hdcMem, hbmOld);
			//DeleteDC(hdcMem);
		}
		EndPaint(hwnd, &ps);
		return 0;
	case WM_MOUSELEAVE:
		return FALSE;
	case WM_MOUSEMOVE:
		return FALSE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

