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
#include "hook.h" // D3D9 hooking class

extern "C" __declspec(dllexport) void entryPoint();

HINSTANCE g_hInstance = NULL;
void displayError(WCHAR* pwszError);
void unloadLibrary();

inline bool validateVersion();

#endif