#ifndef _MAIN_H_
#define _MAIN_H_

#define STRICT
// Need at least Windows XP
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define KERNEL_LIB L"kernel32.dll"

#include <windows.h>
#include <string>
// Raw input class
#include "rawinput.h"
// D3D9 mid-hooking
#include "d3d9hook.h"

extern "C" __declspec(dllexport) void entryPoint();

#endif