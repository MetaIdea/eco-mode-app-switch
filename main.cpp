#pragma once

#ifndef _WIN32_IE
#define _WIN32_IE 0x0600
#endif

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <Windowsx.h>
#include <commctrl.h>
#include <Shellapi.h>
#include <Shlwapi.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "AppStateHandler.h"

#define IDR_MANIFEST                    1
#define IDD_DLG_DIALOG                  102
#define IDD_ABOUTBOX                    103
#define IDM_ABOUT                       104
#define IDC_MYICON						105
#define IDC_STEALTHDIALOG				106
#define IDI_STEALTHDLG                  107
#define IDC_STATI
#define IDI_ICON 1
#define IDC_BUTTON1                     1001
#define IDC_BUTTON2                     1002


#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        110
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1000
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif

#define TRAYICONID	1
#define SWM_TRAYMSG	WM_APP

#define SWM_SHOW	WM_APP + 1
#define SWM_HIDE	WM_APP + 2
#define SWM_EXIT	WM_APP + 3

HINSTANCE		hInst;
NOTIFYICONDATA	niData;
BOOL				InitInstance(HINSTANCE, int);
BOOL				OnInitDialog(HWND hWnd);
void				ShowContextMenu(HWND hWnd);
ULONGLONG			GetDllVersion(LPCTSTR lpszDllName);

INT_PTR CALLBACK	DlgProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	if (!InitInstance(hInstance, nCmdShow)) return FALSE;
	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_STEALTHDIALOG);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg) ||
			!IsDialogMessage(msg.hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)msg.wParam;
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	InitCommonControls();

	hInst = hInstance;
	HWND hWnd = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DLG_DIALOG),NULL, (DLGPROC)DlgProc);

	if (!hWnd) return FALSE;

	ZeroMemory(&niData, sizeof(NOTIFYICONDATA));

	ULONGLONG ullVersion = GetDllVersion(_T("Shell32.dll"));
	if (ullVersion >= MAKEDLLVERULL(5, 0, 0, 0)) {
		niData.cbSize = sizeof(NOTIFYICONDATA);
	}
	else { 
		niData.cbSize = NOTIFYICONDATA_V2_SIZE; 
	}

	niData.uID = TRAYICONID;
	niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	niData.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON),IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),LR_DEFAULTCOLOR);
	niData.hWnd = hWnd;
	niData.uCallbackMessage = SWM_TRAYMSG;
	lstrcpyn(niData.szTip, _T("Eco Mode App Switch - Suspend/Freeze Apps"), sizeof(niData.szTip) / sizeof(TCHAR));
	Shell_NotifyIcon(NIM_ADD, &niData);

	if (niData.hIcon && DestroyIcon(niData.hIcon)) {
		niData.hIcon = NULL;
	}

	return TRUE;
}


BOOL OnInitDialog(HWND hWnd) {
	HMENU hMenu = GetSystemMenu(hWnd, FALSE);
	if (hMenu) {
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, IDM_ABOUT, _T("About"));
	}
	HICON hIcon = (HICON)LoadImage(hInst,MAKEINTRESOURCE(IDI_STEALTHDLG),IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
	SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	return TRUE;
}


void ShowContextMenu(HWND hWnd) {
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if (hMenu) {
		if (IsWindowVisible(hWnd)) {
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_HIDE, _T("Hide"));
		}
		else {
			RefreshAppList(hMenu);
			InsertMenu(hMenu, 999, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
			InsertMenu(hMenu, -1, MF_BYPOSITION, 1000, _T("Donate for Plus Version"));
			InsertMenu(hMenu, -1, MF_BYPOSITION, SWM_EXIT, _T("Exit"));
		}

		SetForegroundWindow(hWnd);
		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
		DestroyMenu(hMenu);
	}
}


ULONGLONG GetDllVersion(LPCTSTR lpszDllName)
{
	ULONGLONG ullVersion = 0;
	HINSTANCE hinstDll;
	hinstDll = LoadLibrary(lpszDllName);
	if (hinstDll)
	{
		DLLGETVERSIONPROC pDllGetVersion;
		pDllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstDll, "DllGetVersion");
		if (pDllGetVersion)
		{
			DLLVERSIONINFO dvi;
			HRESULT hr;
			ZeroMemory(&dvi, sizeof(dvi));
			dvi.cbSize = sizeof(dvi);
			hr = (*pDllGetVersion)(&dvi);
			if (SUCCEEDED(hr))
				ullVersion = MAKEDLLVERULL(dvi.dwMajorVersion, dvi.dwMinorVersion, 0, 0);
		}
		FreeLibrary(hinstDll);
	}
	return ullVersion;
}


INT_PTR CALLBACK DlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message)
	{
	case SWM_TRAYMSG:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_RESTORE);
			break;
		case WM_RBUTTONDOWN:
		case WM_CONTEXTMENU:
			ShowContextMenu(hWnd);
		}
		break;
	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_MINIMIZE)
		{
			ShowWindow(hWnd, SW_HIDE);
			return 1;
		}
		else if (wParam == IDM_ABOUT)
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON1 || LOWORD(wParam) == 1000) {
			ShellExecute(0, L"open", L"https://metaidea.github.io/eco-mode-app-switch/", 0, 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_BUTTON2) {
			ShellExecute(0, L"open", L"https://github.com/MetaIdea/eco-mode-app-switch", 0, 0, SW_SHOWNORMAL);
		}

		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		if (wmId >= APP_LIST_START_ID && wmId < APP_LIST_START_ID + MAX_MENU_ITEMS) {
			SuspendResumeProcess(wmId);
		}
		else {
			switch (wmId)
			{
			case SWM_SHOW:
				ShowWindow(hWnd, SW_RESTORE);
				break;
			case SWM_HIDE:
			case IDOK:
				ShowWindow(hWnd, SW_HIDE);
				break;
			case SWM_EXIT:
				DestroyWindow(hWnd);
				break;
			case IDM_ABOUT:
				DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
				break;
			}
		}
		return 1;
	case WM_INITDIALOG:
		return OnInitDialog(hWnd);
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		niData.uFlags = 0;
		Shell_NotifyIcon(NIM_DELETE, &niData);
		PostQuitMessage(0);
		break;
	}
	return 0;
}


LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		if (LOWORD(wParam) == IDC_BUTTON1) {
			ShellExecute(0, L"open", L"https://metaidea.github.io/eco-mode-app-switch/", 0, 0, SW_SHOWNORMAL);
		}
		if (LOWORD(wParam) == IDC_BUTTON2) {
			ShellExecute(0, L"open", L"https://github.com/MetaIdea/eco-mode-app-switch", 0, 0, SW_SHOWNORMAL);
		}
		break;
	}
	return FALSE;
}