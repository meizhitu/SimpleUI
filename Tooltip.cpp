#define _WIN32_WINNT 0x0501
#define WINVER 0x0501

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "comctl32.Lib")
#pragma comment(lib, "gdi32.Lib")

#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
int count;
TCHAR str[1000];

HWND g_hwndTrackingTT;
TOOLINFO g_toolItem;
HINSTANCE g_hInst;
BOOL g_TrackingMouse = FALSE;

LRESULT CALLBACK WindowFunc(HWND,UINT,WPARAM,LPARAM);

HWND CreateTrackingToolTip(int toolID, HWND hDlg, char *pText);

TCHAR szWinName[] = TEXT("MyWin");

int WINAPI WinMain(HINSTANCE hThisInst,HINSTANCE hPrevInst,LPSTR lpszArgs,int nWinMode)
{
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
	wcl.style = CS_HREDRAW|CS_VREDRAW;
	wcl.hIcon = LoadIcon(NULL,IDI_APPLICATION);
	wcl.hIconSm = NULL;
	wcl.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcl.lpszMenuName = NULL;
	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;
	wcl.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
	if(!RegisterClassEx(&wcl)) return 0;
	hwnd = CreateWindowEx(
		WS_EX_WINDOWEDGE,
		szWinName,
		TEXT("Window Title"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		HWND_DESKTOP,
		NULL,
		hThisInst,
		NULL
		);
	ShowWindow(hwnd,nWinMode);
	UpdateWindow(hwnd);

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}
LRESULT CALLBACK WindowFunc(HWND hwnd,UINT message,WPARAM
							wParam,LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	static int cxClient = 0, cyClient = 0, oldX, oldY;
	int newX, newY;
	char coords[255];
	POINT pt;

	switch(message){
	case WM_CREATE:
		g_hwndTrackingTT = CreateTrackingToolTip(456, hwnd, "");
		return 0;
	case WM_SIZE:
		cxClient = LOWORD(lParam);
		cyClient = HIWORD(lParam);
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		return 0;
	case WM_MOUSELEAVE:
		// The mouse pointer has left our window.
		// Deactivate the ToolTip.
		SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE, (WPARAM)
			FALSE, (LPARAM)&g_toolItem);
		g_TrackingMouse = FALSE;
		return FALSE;
	case WM_MOUSEMOVE:
		if (!g_TrackingMouse)
			// The mouse has just entered the window.
		{
			// Request notification when the mouse leaves.
			TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
			tme.hwndTrack = hwnd;
			tme.dwFlags = TME_LEAVE;
			TrackMouseEvent(&tme);

			// Activate the ToolTip.
			SendMessage(g_hwndTrackingTT, TTM_TRACKACTIVATE,
				(WPARAM)TRUE, (LPARAM)&g_toolItem);
			g_TrackingMouse = TRUE;
		}

		newX = LOWORD(lParam);
		newY = HIWORD(lParam);
		// Make sure the mouse has actually moved. The presence of the ToolTip
		// causes Windows to send the message continuously.
		if ((newX != oldX) || (newY != oldY))
		{
			oldX = newX;
			oldY = newY;
			// Update the text.
			//sprintf(coords, "%d, %d", newX, newY);
			for (int i =0; i< 90; i+=2)
			{
				coords[i] = (char)(i+'0');
				coords[i+1] = '\n';
			}
			coords[90] = '\0';
			g_toolItem.lpszText = coords;
			SendMessage(g_hwndTrackingTT, TTM_SETTOOLINFO, 0, (LPARAM)
				&g_toolItem);

			// Position the ToolTip.
			// The coordinates are adjusted so that the ToolTip does not
			// overlap the mouse pointer.
			pt.x = newX;
			pt.y = newY;
			ClientToScreen(hwnd, &pt);
			SendMessage(g_hwndTrackingTT, TTM_TRACKPOSITION,
				0, (LPARAM)MAKELONG(pt.x + 10, pt.y));
		}
		return FALSE;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd,message,wParam,lParam);
}

HWND CreateTrackingToolTip(int toolID, HWND hDlg, char *pText)
{
	// Create a ToolTip.
	HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS, NULL,
		WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hDlg, NULL, g_hInst,NULL);
	if (!hwndTT)
	{
		return NULL;
	}
	// Set up tool information.
	// In this case, the "tool" is the entire parent window.
	g_toolItem.cbSize = sizeof(TOOLINFO);
	//g_toolItem.cbSize = TTTOOLINFOA_V2_SIZE;
	g_toolItem.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	g_toolItem.hwnd = hDlg;
	g_toolItem.hinst = g_hInst;
	g_toolItem.lpszText = pText;
	g_toolItem.uId = (UINT_PTR)hDlg;
	GetClientRect (hDlg, &g_toolItem.rect);

	// Associate the ToolTip with the tool window.
	SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO)&g_toolItem);
	SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, (LPARAM)(LPTOOLINFO)&g_toolItem);
	return hwndTT;
}
