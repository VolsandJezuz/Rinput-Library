#ifndef _HOOK_H_
#define _HOOK_H_

#include <windows.h>
#include <shlobj.h>

#define PSAPI_VERSION 1
#include <psapi.h>

#pragma intrinsic(memcpy)

#include <string>

extern HINSTANCE g_hInstance;
extern CRITICAL_SECTION d3d9EndMutex;
extern HANDLE dummyEvent;
extern HWND hwndSender;
extern bool bCaptureThreadStop;

bool InitD3D9Capture();
DWORD WINAPI CaptureThread(HANDLE hDllMainThread);
void CheckD3D9Capture();

typedef unsigned long UPARAM;

class HookData
{
	BYTE data[14];
	FARPROC func;
	FARPROC hookFunc;
	bool bHooked;
	bool b64bitJump;
	bool bAttemptedBounce;
	LPVOID bounceAddress;

public:
	inline HookData() : bHooked(false), func(NULL), hookFunc(NULL), b64bitJump(false), bAttemptedBounce(false) {}

	inline bool Hook(FARPROC funcIn, FARPROC hookFuncIn)
	{
		if (bHooked)
		{
			if ((funcIn == func) && (hookFunc != hookFuncIn))
			{
				hookFunc = hookFuncIn;

				Rehook();

				return true;
			}

			Unhook();
		}

		func = funcIn;
		hookFunc = hookFuncIn;

		DWORD oldProtect;

		if (!VirtualProtect((LPVOID)func, 14, PAGE_EXECUTE_READWRITE, &oldProtect))
			return false;

		if (*(BYTE *)func == 0xE9 || *(BYTE *)func == 0xE8)
		{
			CHAR *modName, *ourName;
			CHAR szModName[MAX_PATH];
			CHAR szOurName[MAX_PATH];

			DWORD memAddress;

			MEMORY_BASIC_INFORMATION mem;

			INT_PTR jumpAddress = *(DWORD *)((BYTE *)func + 1) + (DWORD)func;

			if (VirtualQueryEx(GetCurrentProcess(), (LPVOID)jumpAddress, &mem, sizeof(mem)) && mem.State == MEM_COMMIT)
				memAddress = (DWORD)mem.AllocationBase;
			else
				memAddress = jumpAddress;

			if (GetMappedFileNameA(GetCurrentProcess(), (LPVOID)memAddress, szModName, _countof(szModName) - 1))
				modName = szModName;
			else if (GetModuleFileNameA((HMODULE)memAddress, szModName, _countof(szModName) - 1))
				modName = szModName;
			else
				modName = "unknown";

			if (VirtualQueryEx(GetCurrentProcess(), (LPVOID)func, &mem, sizeof(mem)) && mem.State == MEM_COMMIT)
				memAddress = (DWORD)mem.AllocationBase;
			else
				memAddress = (DWORD)func;

			if (GetMappedFileNameA(GetCurrentProcess(), (LPVOID)memAddress, szOurName, _countof(szOurName) - 1))
				ourName = szOurName;
			else if (GetModuleFileNameA((HMODULE)memAddress, szOurName, _countof(szOurName) - 1))
				ourName = szOurName;
			else
				ourName = "unknown";
		}

		memcpy(data, (const void*)func, 14);

		return true;
	}

	inline void Rehook(bool bForce=false)
	{
		if ((!bForce && bHooked) || !func)
			return;

		UPARAM startAddr = UPARAM(func);
		UPARAM targetAddr = UPARAM(hookFunc);

		ULONG64 offset, diff;
		offset = targetAddr - (startAddr + 5);

		if (startAddr + 5 > targetAddr)
			diff = startAddr + 5 - targetAddr;
		else
			diff = targetAddr - startAddr + 5;

		DWORD oldProtect;
		VirtualProtect((LPVOID)func, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
		LPBYTE addrData = (LPBYTE)func;
		*addrData = 0xE9;
		*(DWORD*)(addrData+1) = DWORD(offset);

		bHooked = true;
	}

	inline void Unhook()
	{
		if (!bHooked || !func)
			return;

		UINT count = b64bitJump ? 14 : 5;
		DWORD oldProtect;
		VirtualProtect((LPVOID)func, count, PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy((void*)func, data, count);

		bHooked = false;
	}
};

inline FARPROC GetVTable(LPVOID ptr, UINT funcOffset)
{
	UPARAM *vtable = *(UPARAM**)ptr;
	return (FARPROC)(*(vtable+funcOffset));
}

#endif