#include "stdafx.h"
#include "PWRMeter.h"

#pragma comment(lib,"comctl32.lib")


// Global Variables:
HINSTANCE							hInst;								// current instance
NOTIFYICONDATA				nid;
SYSTEM_POWER_STATUS		status;

// Configuration
DWORD	dwTransparency;
DWORD	dwHeight;
BYTE	bCriticalLevel;

#define CLASS_NAME _T("PWRMeter")

//*****************************************************************************

void LoadConfiguration()
{
	dwTransparency = 128;
	dwHeight = 8;
	bCriticalLevel = 90;
}

//*****************************************************************************

void SaveConfiguration()
{
}

//*****************************************************************************

void DrawBar(HWND hWnd,HDC hDC)
{
	RECT			rcRect;
	COLORREF	crInterior = RGB(0,192,0);
	COLORREF	crBorder = RGB(0,0,0);

	GetWindowRect(hWnd,&rcRect);
	int width = (rcRect.right - rcRect.left) / 100;

	if (status.BatteryLifePercent < 25) crInterior = RGB(192,0,0);
	if (status.ACLineStatus == 1) crInterior = RGB(0,0,255);

	// Create and select chosen colors
	HBRUSH hbrInterior = CreateSolidBrush(crInterior);
	HPEN hpnBorder = CreatePen(PS_SOLID,1,crBorder);
	HGDIOBJ hOldBrush = SelectObject(hDC,hbrInterior);
	HGDIOBJ hOldPen = SelectObject(hDC,hpnBorder);

	RECT rcBlock;
	rcBlock.left = 0;
	rcBlock.right = width - 1;
	rcBlock.top = 1;
	rcBlock.bottom = rcRect.bottom - 1;

	for (int i = 0;i < status.BatteryLifePercent;i++)
	{
		Rectangle(hDC,rcBlock.left,rcBlock.top,rcBlock.right,rcBlock.bottom);
		rcBlock.left+= width;
		rcBlock.right+= width;
	}

	// Now clear remaining space
	rcBlock.right = rcRect.right;
	SelectObject(hDC,GetStockObject(NULL_PEN));
	HBRUSH hbrBack = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	SelectObject(hDC,hbrBack);
	Rectangle(hDC,rcBlock.left,rcRect.top,rcBlock.right,rcRect.bottom);

	// Normalize
	SelectObject(hDC,hOldBrush);
	SelectObject(hDC,hOldPen);
	DeleteObject(hbrInterior);
	DeleteObject(hpnBorder);
	DeleteObject(hbrBack);
}

//*****************************************************************************

void HandleNotification(HWND hWnd,WORD wMsg)
{
	HMENU	hMenu;
	HMENU	hSubMenu;
	POINT	pos;

	switch(wMsg)
	{
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			hMenu = ::LoadMenu(hInst,MAKEINTRESOURCE(IDR_POPUP));
			if (!hMenu) return;

			hSubMenu = ::GetSubMenu(hMenu,0);

			// Make chosen menu item the default (bold font)
			SetMenuDefaultItem(hSubMenu,0,TRUE);

			// Display and track the popup menu
			GetCursorPos(&pos);

			SetForegroundWindow(hWnd);
			TrackPopupMenu(hSubMenu,0,pos.x,pos.y,0,hWnd,0);

			// BUGFIX: See "PRB: Menus for Notification Icons Don't Work Correctly"
			::PostMessage(nid.hWnd,WM_NULL,0,0);

			DestroyMenu(hMenu);
			break;

		case WM_LBUTTONDBLCLK:
			ShellExecute(0,_T("open"),_T("rundll32.exe"),_T("shell32.dll,Control_RunDLL powercfg.cpl,,0"),_T("."),SW_RESTORE);
			break;
	}
}

//*****************************************************************************

void UpdateTip()
{
	LPTSTR	pACStatus;
	TCHAR		szLevel[128];
	LPTSTR	pBatteryStatus;

	switch (status.ACLineStatus)
	{
	case 0:
		pACStatus = _T("On Battery");
		break;
	case 1:
		pACStatus = _T("On AC");
		break;
	default:
		pACStatus = _T("AC status unknown");
		break;
	}

	if (status.BatteryFlag & 0x80)
	{
		pBatteryStatus = _T("No battery detected");
	}
	else
		if (status.BatteryLifePercent == 255)
		{
			pBatteryStatus = _T("Unknown");
		}
	else
	{
		wsprintf(szLevel,_T("Battery level: %d%%"),status.BatteryLifePercent);
		pBatteryStatus = szLevel;
	}

	wsprintf(nid.szTip,_T("%s\n%s"),pACStatus,pBatteryStatus);
	nid.uFlags = NIF_TIP;
	Shell_NotifyIcon(NIM_MODIFY,&nid);
}

//*****************************************************************************

BOOL CALLBACK ConfigProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam)
{
	HWND hCtrl;

	switch (message)
	{
		case WM_INITDIALOG:
			hCtrl = GetDlgItem(hwndDlg,IDC_TRANSPARENCY_SLD);
			SendMessage(hCtrl,TBM_SETRANGE,(WPARAM) FALSE,(LPARAM) MAKELONG(0,16));
			SendMessage(hCtrl,TBM_SETPOS,(WPARAM) TRUE,(LPARAM)(dwTransparency / 16));

			hCtrl = GetDlgItem(hwndDlg,IDC_HEIGHT_SLD);
			SendMessage(hCtrl,TBM_SETRANGE,(WPARAM) FALSE,(LPARAM) MAKELONG(1,8));
			SendMessage(hCtrl,TBM_SETPOS,(WPARAM) TRUE,(LPARAM)(dwHeight / 4));

			hCtrl = GetDlgItem(hwndDlg,IDC_CRITICALLEVEL_SLD);
			SendMessage(hCtrl,TBM_SETRANGE,(WPARAM) FALSE,(LPARAM) MAKELONG(0,9));
			SendMessage(hCtrl,TBM_SETPOS,(WPARAM) TRUE,(LPARAM)((bCriticalLevel - 50) / 5));

			break;

		case WM_COMMAND: 
			switch (LOWORD(wParam))
			{
				case IDOK:
					hCtrl = GetDlgItem(hwndDlg,IDC_TRANSPARENCY_SLD);
					dwTransparency = (DWORD)SendMessage(hCtrl,TBM_GETPOS,0,0) * 16;

					hCtrl = GetDlgItem(hwndDlg,IDC_HEIGHT_SLD);
					dwHeight = SendMessage(hCtrl,TBM_GETPOS,0,0) * 4;

					hCtrl = GetDlgItem(hwndDlg,IDC_CRITICALLEVEL_SLD);
					bCriticalLevel = (BYTE)(SendMessage(hCtrl,TBM_GETPOS,0,0) * 5) + 50;

				case IDCANCEL:
					EndDialog(hwndDlg,wParam);
					return TRUE;
			}
	}

	return FALSE;
}

//*****************************************************************************

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT	ps;
	HDC					hdc;
	BYTE				oldStatus;

	switch (message)
	{
		case WM_CREATE:
			SetWindowLong(hWnd,GWL_STYLE,GetWindowLong(hWnd,GWL_STYLE) & ~(WS_DLGFRAME | WS_BORDER));
			SetWindowPos(hWnd,0,0,0,0,0,SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
			SetLayeredWindowAttributes(hWnd,GetSysColor(COLOR_WINDOW),dwTransparency,LWA_COLORKEY | LWA_ALPHA);

			// Create shell icon
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hIcon = (HICON)LoadImage(hInst,MAKEINTRESOURCE(IDI_CHARGED),IMAGE_ICON,0,0,LR_DEFAULTCOLOR);
			nid.hWnd = hWnd;
			nid.uID = 1;
			nid.uCallbackMessage = WM_APP;
			nid.uFlags = NIF_ICON | NIF_MESSAGE;
			Shell_NotifyIcon(NIM_ADD,&nid);

			GetSystemPowerStatus(&status);
			UpdateTip();

			SetTimer(hWnd,1,1000,0);
			break;

		case WM_PAINT:
			hdc = BeginPaint(hWnd, &ps);
			GetSystemPowerStatus(&status);
			DrawBar(hWnd,hdc);
			EndPaint(hWnd,&ps);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_MENU_ADJUSTPOWERPROPERTIES:
					ShellExecute(0,_T("open"),_T("rundll32.exe"),_T("shell32.dll,Control_RunDLL powercfg.cpl,,0"),_T("."),SW_RESTORE);
					break;

				case ID_MENU_CONFIGURE:
					if (DialogBox(hInst,MAKEINTRESOURCE(IDD_PWRMETER_DIALOG),hWnd,(DLGPROC)ConfigProc) == IDOK)
					{
						SaveConfiguration();
						SetWindowPos(hWnd,0,0,0/*y*/,GetSystemMetrics(SM_CXFULLSCREEN),dwHeight,SWP_NOZORDER);
						SetLayeredWindowAttributes(hWnd,GetSysColor(COLOR_WINDOW),dwTransparency,LWA_COLORKEY | LWA_ALPHA);
					}
					break;

				case ID_MENU_EXIT:
					DestroyWindow(hWnd);
					break;
			}
			break;

		case WM_TIMER:
			oldStatus = status.BatteryLifePercent;
			GetSystemPowerStatus(&status);
			if (oldStatus != status.BatteryLifePercent)
			{
				hdc = GetDC(hWnd);
				DrawBar(hWnd,hdc);
				ReleaseDC(hWnd,hdc);
				UpdateTip();
			}
			break;

		case WM_APP:
			HandleNotification(hWnd,LOWORD(lParam));
			break;

		case WM_NCHITTEST:
			return HTTRANSPARENT;

		case WM_DESTROY:
			Shell_NotifyIcon(NIM_DELETE,&nid);
			KillTimer(hWnd,1);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

//*****************************************************************************

int APIENTRY _tWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPTSTR lpCmdLine,int nCmdShow)
{
	WNDCLASSEX	wcex;
	MSG					msg;

	hInst = hInstance;

	InitCommonControls();

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= 0;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= CLASS_NAME;
	wcex.hIconSm		= 0;

	// If failed then must be already running
	if (RegisterClassEx(&wcex) == 0) return 1;

	// Warn user if a battery is not present
	GetSystemPowerStatus(&status);
	if (status.BatteryFlag & 0x80)
	{
		TCHAR szMsg[255];
		LoadString(hInstance,IDS_NOBATTERY,szMsg,sizeof(szMsg) / sizeof(TCHAR));
		if (MessageBox(0,szMsg,CLASS_NAME,MB_YESNO) == IDNO) return 1;
	}

	LoadConfiguration();
	int width = GetSystemMetrics(SM_CXFULLSCREEN);
	HWND hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT,
															CLASS_NAME,"",WS_OVERLAPPED,0,0,width,dwHeight,0,0,hInstance,0);
	if (!hWnd) return FALSE;

	ShowWindow(hWnd,nCmdShow);
	UpdateWindow(hWnd);

	// Main message loop:
	while (GetMessage(&msg,0,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
