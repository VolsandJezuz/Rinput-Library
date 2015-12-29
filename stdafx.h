#pragma once

#define STRICT
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define KERNEL_LIB L"kernel32.dll"
#define RINPUTVER "v1.43"
#define RINPUTFVER 1,43
#define TF2 2
#define NOBUGFIXES 4
#define MAX_CONSEC_ENDSCENE 7
#define MAX_CONSECG 3

#include <windows.h>

extern int consec_EndScene;
extern HANDLE hD3D9HookThread;
extern int n_sourceEXE;

DWORD WINAPI D3D9HookThread(LPVOID lpParameter);

class CRawInput
{
public:
	// Initialize RInput components, register for raw input
	static bool initialize(WCHAR* pwszError);
	static bool initWindow(WCHAR* pwszError);
	static bool initInput(WCHAR* pwszError);
	// Identify main visible window of the injected process
	static BOOL CALLBACK EnumWindowsProc(HWND WindowHandle, LPARAM lParam);
	static HWND clientWindow(unsigned long PID);
	// Get coords of the injected process' window center
	static bool clientCenter();
	// Poll mouse input
	static unsigned int pollInput();
	// Mouse input window proc
	static LRESULT __stdcall wpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	// Hooked cursor functions handling
	static int __stdcall hSetCursorPos(int x, int y);
	static int __stdcall hGetCursorPos(LPPOINT lpPoint);
	// TF2 subclassing and input blocking when alt-tabbing back in
	static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	static DWORD WINAPI blockInput(LPVOID lpParameter);
	// Enables or disables hooking and mouse critical section
	static bool hookLibrary(bool bInstall);
	// Unload RInput components, stop raw input reads from mouse
	static void unload();

private:
	static HWND hwndClient;
	static bool TF2unblock;
	static long hold_x;
	static long hold_y;
	static POINT centerPoint;
	static HWND hwndInput;
	static long set_x;
	static long set_y;
	static bool bRegistered;
	static CRITICAL_SECTION rawMouseData;
	static long x;
	static long y;
	static int signal;
	static int consecG;
	static bool alttab;
	static int SCP;
	static bool bSubclass;
	static HANDLE hCreateThread;
};