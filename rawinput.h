#ifndef RAWINPUT_H_
#define RAWINPUT_H_

#define TF2 2
#define NO_BUG_FIXES 4
#define MAX_CONSEC_ENDSCENE 7
#define MAX_CONSECG 3

#include <windows.h>
#include "detours.h"
#include <string>
#include <d3d9.h>
#include <shlwapi.h>
#include <commctrl.h>

extern void displayError(WCHAR* pwszError);

class CRawInput
{
public:
	// Initialize RInput components, register for raw input
	static bool initialize(WCHAR* pwszError);
	static bool initWindow(WCHAR* pwszError);
	static bool initInput(WCHAR* pwszError);
	// Mouse input window message processing
	static LRESULT CALLBACK wpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	// Enables or disables hooking and mouse critical section
	static bool hookLibrary(bool bInstall);
	// Hooked cursor functions handling
	static BOOL WINAPI hGetCursorPos(LPPOINT lpPoint);
	static BOOL WINAPI hSetCursorPos(int x, int y);
	// Poll mouse input
	static UINT pollInput();
	// Unload RInput components, stop raw input reads from mouse
	static void unload();

private:
	static unsigned char n_sourceEXE;
	static HWND hwndClient;
	static bool TF2unblock;
	static POINT centerPoint;
	static long leftBoundary;
	static long rightBoundary;
	static long topBoundary;
	static long bottomBoundary;
	static long hold_x;
	static long hold_y;
	static CRITICAL_SECTION rawMouseData;
	static long x;
	static long y;
	static HWND hwndInput;
	static long set_x;
	static long set_y;
	static bool bRegistered;
	static unsigned char signal;
	static bool bSubclass;
	static HANDLE hCreateThread;
	static unsigned char consecG;
	static bool alttab;
	static unsigned char consec_EndScene;
	static char SCP;
	static HANDLE hD3D9HookThread;
	static DWORD oD3D9EndScene;
	// Identify main visible window of the injected process
	static BOOL CALLBACK EnumWindowsProc(HWND WindowHandle, LPARAM lParam);
	// Get coords of the injected process' window center
	static bool clientCenter();
	// TF2 subclassing and input blocking when alt-tabbing back in
	static LRESULT CALLBACK SubclassWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	static DWORD WINAPI blockInput(LPVOID lpParameter);
	// D3D9 hooking for CS:GO and TF2
	static DWORD WINAPI D3D9HookThread(LPVOID lpParameter);
	static PDWORD vtableFind(DWORD D3D9Base);
	static void JMPplace(BYTE* inFunc, DWORD deFunc, DWORD len);
	static HRESULT WINAPI D3D9EndScene();
};

#endif