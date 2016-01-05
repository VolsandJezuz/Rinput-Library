#ifndef _MAIN_H_
#define _MAIN_H_

#define STRICT
#define WINVER 0x0501
#define _WIN32_WINDOWS 0x0501
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define KERNEL_LIB L"kernel32.dll"

#include "rawinput.h"

extern "C" __declspec(dllexport) void entryPoint();

void displayError(WCHAR* pwszError);

#endif