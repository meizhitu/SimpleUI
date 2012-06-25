#include <windows.h>
#pragma comment(lib,"kernel32")
#pragma comment(lib,"user32")
#define WM_EXITQDESK 10086

RECT g_rcTrayWnd;
RECT g_rcRebar;
HWND g_trayWnd;
HWND g_rebar;
WNDPROC g_oldProc = NULL;
//typedef struct tagWINDOWPOS {
//  HWND hwnd;
//  HWND hwndInsertAfter;
//  int  x;
//  int  y;
//  int  cx;
//  int  cy;
//  UINT flags;
//} WINDOWPOS, *LPWINDOWPOS, *PWINDOWPOS;
LRESULT APIENTRY NewWndProc(HWND hWnd, UINT wMessage , WPARAM wParam, LPARAM lParam)
{
	switch (wMessage)
	{
	case WM_WINDOWPOSCHANGING:
		{
			//		::SetWindowPos(g_rebar, NULL, g_rcTrayWnd.left+112, 0,   
			//g_rcRebar.right - g_rcTrayWnd.left-112, g_rcRebar.bottom - g_rcRebar.top,   
			//SWP_NOZORDER|SWP_NOACTIVATE);
			LPWINDOWPOS wndPos =(LPWINDOWPOS)lParam;
			wndPos->x = g_rcTrayWnd.left+112;
		}
		break;
	case WM_EXITQDESK:
		{
			if (NULL != g_oldProc)
				::SetWindowLongPtr(g_rebar,GWLP_WNDPROC,(LONG_PTR)g_oldProc);
				g_oldProc = NULL;
		}
		break;
	default:
		{
			if (NULL == g_oldProc)
				return DefWindowProc(hWnd,wMessage,wParam,lParam);
			else
				return CallWindowProc(g_oldProc,hWnd,wMessage,wParam,lParam);
		}
	}
}
BOOL APIENTRY DllMain( HANDLE hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{	
	if( (ul_reason_for_call == DLL_PROCESS_ATTACH)) {
		g_trayWnd = ::FindWindow("Shell_TrayWnd", NULL);
		g_rebar = ::FindWindowEx(g_trayWnd, NULL, "ReBarWindow32", NULL);
		g_oldProc=(WNDPROC)::SetWindowLongPtr(g_rebar,GWLP_WNDPROC,(LONG_PTR)NewWndProc);	
	}	
	return TRUE;
}
