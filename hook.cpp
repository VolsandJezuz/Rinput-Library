/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and are only called for CS:GO.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"

HWND hwndSender = NULL;
static bool bD3D9Hooked = false;
HANDLE dummyEvent = NULL;
CRITICAL_SECTION d3d9EndMutex;

inline bool AttemptToHookSomething()
{
	if (!hwndSender)
		return false;

	bool bFoundSomethingToHook = false;

	if (!bD3D9Hooked && InitD3D9Capture())
	{
		bFoundSomethingToHook = true;

		bD3D9Hooked = true;
	}
	else
		CheckD3D9Capture();
	
	return bFoundSomethingToHook;
}

static inline HWND CreateDummyWindow(LPCTSTR lpClass, LPCTSTR lpName)
{
	return CreateWindowEx(0, lpClass, lpName, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, NULL, NULL, g_hInstance, NULL);
}

static DWORD WINAPI DummyWindowThread(LPVOID lpBla)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = g_hInstance;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	wc.lpszClassName = TEXT("RInputD3DSender");

	if (RegisterClass(&wc))
	{
		hwndSender = CreateDummyWindow(TEXT("RInputD3DSender"), NULL);

		if (!hwndSender)
			return 0;
	}
	else
		return 0;

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

DWORD WINAPI CaptureThread(HANDLE hDllMainThread)
{
	bool bSuccess = false;

	if (hDllMainThread)
	{
		WaitForSingleObject(hDllMainThread, 150);
		CloseHandle(hDllMainThread);
	}

	dummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	InitializeCriticalSection(&d3d9EndMutex);

	DWORD bla;
	HANDLE hWindowThread = CreateThread(NULL, 0, DummyWindowThread, NULL, 0, &bla);

	if (!hWindowThread)
		return 0;

	CloseHandle(hWindowThread);

	while(!AttemptToHookSomething())
		Sleep(50);

	for (size_t n = 0; !bCaptureThreadStop; ++n)
	{
		// Less frequent loop interations reduce resource usage
		if (n % 16 == 0)
			AttemptToHookSomething();

		// Overall interval for checking EndScene hook is still 4000ms
		Sleep(250);
	}

	DeleteCriticalSection(&d3d9EndMutex);

	return 0;
}