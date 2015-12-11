#ifndef _MAIN_H_
#define _MAIN_H_

#define STRICT
// Need at least Windows XP
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
// Amount of bytes to store an error string
#define ERROR_BUFFER_SIZE 256
#define EVENTNAME "RInputEvent32"
#define KERNEL_LIB L"kernel32.dll"

// Raw input class
#include "rawinput.h"
// D3D hooking class
#include "hook.h"

extern "C" __declspec(dllexport) void entryPoint();

void displayError(WCHAR* pwszError);
void unloadLibrary();

inline bool validateVersion();

#endif