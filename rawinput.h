#ifndef RAWINPUT_H_
#define RAWINPUT_H_

#include <windows.h>
#include "detours.h"

#define RAWINPUTHDRSIZE sizeof(RAWINPUTHEADER)
#define RAWPTRSIZE 40
#define INPUTWINDOW "RInput"

// Prevent warning level 4 warnings for detouring
#pragma warning(disable: 4100)

extern HWND hwndClient;
extern bool sourceEXE;
extern int n_sourceEXE;

/**
 * Note from original author (abort):
 * ----------------------------------
 * Sadly everything has been made static, as Win32 API does not support object oriented callbacks, as it has been written in C.
 * To keep the performance as high as possible, I decided not to work with storing the class instance through Win32 API. 
 * Feel free to rewrite this to something more clean in coding terms :).
 */
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
	// Enables or disables the mouse hooking and CRITICAL_SECTION
	static bool hookLibrary(bool bInstall);
	// Unload RInput components, stop raw input reads from mouse
	static void unload();

private:
	static HWND hwndInput;
	static bool TF2unblock;
	static long hold_x;
	static long hold_y;
	static long set_x;
	static long set_y;
	static bool bRegistered;
	static POINT centerPoint;
	static long x;
	static long y;
	static int signal;
	static int consecG;
	static bool alttab;
	static int SCP;
	static bool bSubclass;
	static HANDLE hCreateThread;
};

#endif