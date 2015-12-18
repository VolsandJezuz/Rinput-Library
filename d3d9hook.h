#ifndef _D3D9HOOK_H_
#define _D3D9HOOK_H_

#include <windows.h>

extern HANDLE hD3D9HookThread;
extern int consec_EndScene;

DWORD WINAPI D3D9HookThread(LPVOID lpParameter);

#endif