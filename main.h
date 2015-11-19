#ifndef _MAIN_H_
#define _MAIN_H_

#define STRICT
#define WINVER 0x0501 // Need at least Windows XP
#define _WIN32_WINDOWS 0x0501 // Need at least Windows XP
#define _WIN32_WINNT 0x0501 // Need at least Windows XP
#define WIN32_LEAN_AND_MEAN

#define ERROR_BUFFER_SIZE 256 // Amount of bytes to store an error string

#define EVENTNAME "RInputEvent32"
#define KERNEL_LIB L"kernel32.dll"

#include "rawinput.h" // Raw input class
#include "hook.h" // D3D9 hooking

HINSTANCE g_hInstance = NULL;
int n_sourceEXE = 0;
bool sourceEXE = false;
int consec_endscene = 0;

// Only handle the hooking and dll functions here
extern "C" __declspec(dllexport) void entryPoint();
inline bool validateVersion();
void unloadLibrary();
void displayError(WCHAR* pwszError);

#endif