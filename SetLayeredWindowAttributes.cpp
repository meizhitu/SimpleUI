#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <gdiplus.h>
using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Ole32.lib")

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.Lib")
#pragma comment(lib, "gdi32.Lib")

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE g_hInst;
HBITMAP g_hbmBall = NULL;

LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM,LPARAM);

TCHAR szWinName[] = TEXT("MyWin");

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
	HWND parent = FindWindow("Shell_TrayWnd", "");
	hwnd = CreateWindowEx(
		0x00080080,
		szWinName,
		TEXT("Window Title"),
		0x16080000,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		120,
		150,
		parent,//HWND_DESKTOP
		NULL,
		hThisInst,
		NULL
		);
		
	::SetLayeredWindowAttributes( hwnd, RGB(0,0,255), 200, ULW_ALPHA|ULW_COLORKEY);
	ShowWindow(hwnd,nWinMode);
	UpdateWindow(hwnd);

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
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
			Image img(L"µ×Í¼.png");
			g.DrawImage( &img, 0, 0);
    	
    	
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
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

