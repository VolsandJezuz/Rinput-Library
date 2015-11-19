/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and are only called for CS:GO.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"
#include <psapi.h>

HWND hwndSender = NULL;
bool bTargetAcquired = false;
extern bool bCaptureThreadStop;
extern HANDLE hCaptureThread;
CRITICAL_SECTION d3d9EndMutex;
void CheckD3D9Capture();
bool bD3D9Hooked = false;
HANDLE dummyEvent = NULL;

inline bool AttemptToHookSomething()
{
	if (!hwndSender)
		return false;

	bool bFoundSomethingToHook = false;
	if (!bD3D9Hooked)
	{
		if (InitD3D9Capture())
		{
			bFoundSomethingToHook = true;
			bD3D9Hooked = true;
		}
	}
	else
		CheckD3D9Capture();
	
	return bFoundSomethingToHook;
}

#define SENDER_WINDOWCLASS TEXT("RInputD3DSender")

#define TIMER_ID 431879
BOOL GetMessageTimeout(MSG &msg, DWORD timeout)
{
    BOOL ret;
    SetTimer(NULL, TIMER_ID, timeout, NULL);
    ret = GetMessage(&msg, NULL, 0, 0);
    KillTimer(NULL, TIMER_ID);
    return ret;
}

static inline HWND CreateDummyWindow(LPCTSTR lpClass, LPCTSTR lpName)
{
	return CreateWindowEx (0, lpClass, lpName, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 1, 1, NULL, NULL, g_hInstance, NULL);
}

static DWORD WINAPI DummyWindowThread(LPVOID lpBla)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.style = CS_OWNDC;
	wc.hInstance = g_hInstance;
	wc.lpfnWndProc = (WNDPROC)DefWindowProc;
	
	wc.lpszClassName = SENDER_WINDOWCLASS;
	if (RegisterClass(&wc))
	{
		hwndSender = CreateDummyWindow(SENDER_WINDOWCLASS, NULL);
		if (!hwndSender)
			return 0;
		else
			return 0;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

DWORD WINAPI CaptureThread(HANDLE hEntryPointThread)
{
	bool bSuccess = false;

	if (hEntryPointThread)
	{
		WaitForSingleObject(hEntryPointThread, 150);
		CloseHandle(hEntryPointThread);
	}

	dummyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	InitializeCriticalSection(&d3d9EndMutex);

	DWORD bla;
	HANDLE hWindowThread = CreateThread(NULL, 0, DummyWindowThread, NULL, 0, &bla);
	if (!hWindowThread)
		return 0;

	CloseHandle(hWindowThread);

	while(!AttemptToHookSomething())
	{
		Sleep(50);
	}

	for (size_t n = 0; !bCaptureThreadStop; ++n)
	{
		// Less frequent loop interations reduce system resource usage
		if (n % 16 == 0) AttemptToHookSomething();
		// Overall interval for checking EndScene hook is still 4000ms
		Sleep(250);
	}

	return 0;
}

static bool hooking = true;

typedef BOOL (WINAPI *UHWHEXPROC)(HHOOK);

extern "C" __declspec(dllexport) LRESULT CALLBACK DummyDebugProc(int code, WPARAM wParam, LPARAM lParam)
{
	MSG *msg = (MSG*)lParam;

	if (hooking && msg->message == (WM_USER + 432))
	{
		char uhwhexStr[20];
		HMODULE hU32 = GetModuleHandleW(L"USER32");

		memcpy(uhwhexStr, "PjoinkTkch`yz@de~Qo", 20);

		for (int i = 0; i < 19; i++) uhwhexStr[i] ^= i ^ 5;

		UHWHEXPROC unhookWindowsHookEx = (UHWHEXPROC)GetProcAddress(hU32, uhwhexStr);
		if (unhookWindowsHookEx)	   
			unhookWindowsHookEx((HHOOK)msg->lParam);
		hooking = false;
	}

	return CallNextHookEx(0, code, wParam, lParam);
}