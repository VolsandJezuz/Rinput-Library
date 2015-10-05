#ifndef RAWINPUT_H_
#define RAWINPUT_H_

#include <windows.h>
#include "detours.h" // Required for function hooking

#define RAWINPUTHDRSIZE sizeof(RAWINPUTHEADER)
#define RAWPTRSIZE 40
#define INPUTWINDOW "RInput"

// Prevent warning level 4 warnings for detouring
#pragma warning(disable: 4100)

/**
 * Note from original author (abort):
 * ----------------------------------
 * Sadly everything has been made static, as Win32 API does not support object oriented callbacks, as it has been written in C.
 * To keep the performance as high as possible, I decided not to work with storing the class instance through Win32 API. 
 * Feel free to rewrite this to something more clean in coding terms :).
 */
class CRawInput {
public:
	// Initialize raw input
	static bool initialize(WCHAR* pwszError);

	// Enable/Disable the hooking
	static bool hookLibrary(bool bInstall);

	// Hooked functions handling
	static int __stdcall hGetCursorPos(LPPOINT lpPoint);
	static int __stdcall hSetCursorPos(int x, int y);

	// Poll Input
	static unsigned int pollInput();

	// Input Window Proc
	static LRESULT __stdcall wpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	// Initialization
	static bool initWindow(WCHAR* pwszError);
	static bool initInput(WCHAR* pwszError);

	// Unload Raw Input
	static void unload();

private:
	static long x;
	static long y;
	static long set_x;
	static long set_y;

	static HWND hwndInput;
	static bool bRegistered;
};

extern void displayError(WCHAR* pwszError);

#endif