// #define PING_KUN_DEBUG 1
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shellapi.h>
#include <strsafe.h>
#include <functional>

#include "../resource.h"
#include "Win32Mutex.hpp"
#include "TaskTrayNotifyIcon.hpp"
#include "Ping.hpp"


#define L_APPNAME L"PingIndicator"

TaskTrayNotifyIcon ni;


static DWORD WINAPI threadFunc(void* param) {
	const char* address = "8.8.8.8";
	const int	TimerPeriodInMilliSeconds	= 1 * 1000;

	HINSTANCE const hInstance = GetModuleHandle(NULL);
	HICON const hGreenIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
	HICON const hRedIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));

	ni.setTipText(L_APPNAME);
	ni.setIcon(hGreenIcon);

	Ping target;
	bool prevSuccess = true;
	for(;;) {
#if defined(PING_KUN_DEBUG)
		auto result = target.ping(address);
		if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
			result = -1;
		}
#else
		const auto result = target.ping(address);
#endif

		const bool success = (result >= 0);
		const bool update = (success != prevSuccess);
		prevSuccess = success;

		if(update) {
			if(prevSuccess) {
				ni.setTipText(L_APPNAME);
				ni.setIcon(hGreenIcon);
			} else {
				SYSTEMTIME t;
				GetLocalTime(&t);
				const size_t bufLen = 256;
				wchar_t buf[bufLen];
				swprintf_s(
					  buf
					, bufLen
					, L_APPNAME ": First failed %02d/%02d %02d:%02d:%02d"
					, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond
				);
				ni.setTipText(buf);
				ni.setIcon(hRedIcon);
			}
		}
		Sleep(TimerPeriodInMilliSeconds);
	}
}


static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static const UINT WM_MY_TASKTRAY = WM_USER + 1;
	static const UINT CM_MY_EXIT	 = 1;

	switch(uMsg) {
	default:
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_CREATE:
		ni.setCallback(hWnd, WM_MY_TASKTRAY);
		CreateThread(0, 0, threadFunc, 0, 0, 0);
		return 0;

	case WM_MY_TASKTRAY:
		switch(lParam) {
		default:
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			return ni.messageHandler([&](HMENU hMenu) {
				AppendMenuW(hMenu, MF_STRING, CM_MY_EXIT, L"Quit");
			});
		}
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		default:
			break;
		case CM_MY_EXIT:
			PostQuitMessage(0);
			break;
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
	Win32Mutex mutex;
	if(! mutex.create(L"Global" L_APPNAME)) {
		OutputDebugStringW(L_APPNAME ": mutex.create() failed");
		return 0;
	}

	WNDCLASSW wc {};
	wc.lpfnWndProc   = wndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = L_APPNAME "MainWindowClass";
	RegisterClassW(&wc);

	const HWND hWnd = CreateWindowExW(
		WS_EX_TOOLWINDOW, wc.lpszClassName, L_APPNAME,
		0, 0, 0, 0, 0, 0, 0, wc.hInstance, nullptr);

	SetProcessWorkingSetSize(
		  GetCurrentProcess()
		, static_cast<SIZE_T>(-1)
		, static_cast<SIZE_T>(-1)
	);

	MSG msg;
	while(GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	ni.cleanup();

	return 0;
}
