#include "d3d9hook.h"
#include <shlobj.h>
#include <d3d9.h>

DWORD oD3D9EndScene = 0;

DWORD vtableAdd(int num, DWORD D3D9Base)
{
	PDWORD vtabl;

	for (DWORD g = 0; g < 0x1c3000; ++g)
	{
		++D3D9Base;

		// Alternative to signature search with FindPattern
		if ((*(WORD*)(D3D9Base + 0x00)) == 0x06C7 && (*(WORD*)(D3D9Base + 0x06)) == 0x8689 && (*(WORD*)(D3D9Base + 0x0C)) == 0x8689)
		{
			D3D9Base += 2;
			break;
		}
		else if (g >= 0x1c3000)
			return 0;
	}

	*(DWORD*)&vtabl = *(DWORD*)D3D9Base;
	return vtabl[num];
}

// Midhook avoids access violations if D3D9 is hooked by another program
void JMPplace(BYTE* inFunc, DWORD deFunc, DWORD len)
{
	DWORD oldPro = 0;
	VirtualProtect(inFunc, len, PAGE_EXECUTE_READWRITE, &oldPro);
	DWORD relAddress = 0;
	relAddress = (deFunc - (DWORD)inFunc) - 5;
	*inFunc = 0xE9;
	*((DWORD *)(inFunc + 0x1)) = relAddress;
	
	for (DWORD x = 0x5; x < len; x++)
		*(inFunc + x) = 0x90;
	
	DWORD newPro = 0;
	VirtualProtect(inFunc, len, oldPro, &newPro);
}

__declspec(naked) HRESULT __stdcall D3D9EndScene()
{
	static LPDIRECT3DDEVICE9 pDevice;

	__asm mov edi, edi
	__asm push ebp
	__asm mov ebp, esp
	__asm mov eax, dword ptr ss : [ebp + 0x8]
	__asm mov pDevice, eax
	__asm pushad

	if (consec_EndScene < 6)
		++consec_EndScene;

	__asm popad
	__asm jmp[oD3D9EndScene]
}

DWORD WINAPI D3D9HookThread(LPVOID lpParameter)
{
	DWORD vtableBase = 0;
	HMODULE hD3D9Dll = NULL;

	while (!hD3D9Dll)
	{
		hD3D9Dll = GetModuleHandleA("d3d9.dll");
		Sleep(150);
	}
	
	if (vtableBase = vtableAdd(42, (DWORD)hD3D9Dll))
	{
		// D3D9 device vtable found
		oD3D9EndScene = vtableBase + 5;
		JMPplace((PBYTE)(oD3D9EndScene - 5), (DWORD)D3D9EndScene, 5);
	}

	CloseHandle(hD3D9HookThread);
	return 1;
}