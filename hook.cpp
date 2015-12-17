/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and for fixing the bugs in the TF2 MVM
 Upgrade Station menu and some of the bugs in TF2's backpack.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"

HWND hwndSender = NULL;
bool bD3D9Hooked = false;
static HANDLE dummyEvent = NULL;

inline bool AttemptToHookSomething()
{
	bool bFoundSomethingToHook = false;

	// Creates D3D9Ex device for invisible dummy window
	if (!bD3D9Hooked && InitD3D9Capture())
	{
		// InitD3D9Capture returned true, so D3D9 hooking was successful
		bFoundSomethingToHook = true;
		bD3D9Hooked = true;

		// Dummy window no longer need, so its resources can be freed
		if (dummyEvent)
			CloseHandle(dummyEvent);

		if (hwndSender)
			DestroyWindow(hwndSender);
	}
	
	return bFoundSomethingToHook;
}

inline HWND CreateDummyWindow(LPCTSTR lpClass, LPCTSTR lpName)
{
	return CreateWindowEx(0, lpClass, lpName, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, NULL, NULL, g_hInstance, NULL);
}

// Creates an invisible window and process its messages
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

	while (GetMessage(&msg, NULL, 0, 0) != 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

// Starting point for D3D hooking
DWORD WINAPI CaptureThread(LPVOID lpParameter)
{
	dummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	DWORD bla;
	HANDLE hWindowThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)DummyWindowThread, NULL, 0, &bla);

	if (!hWindowThread)
	{
		CloseHandle(hCaptureThread);
		return 0;
	}

	CloseHandle(hWindowThread);

	// Loop to attempt D3D hooking until it is successful
	while(!AttemptToHookSomething())
		Sleep(50);

	CloseHandle(hCaptureThread);
	return 1;
}