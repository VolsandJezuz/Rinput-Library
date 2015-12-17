/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and for fixing the bugs in the TF2 MVM
 Upgrade Station menu and some of the bugs in TF2's backpack.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"
#include <shlobj.h>
#include <d3d9.h>

CRITICAL_SECTION d3d9EndMutex;
HookData d3d9EndScene;

typedef HRESULT(STDMETHODCALLTYPE *D3D9EndScenePROC)(IDirect3DDevice9 *device);

HRESULT STDMETHODCALLTYPE D3D9EndScene(IDirect3DDevice9 *device)
{
	HRESULT hRes = E_FAIL;

	// Make sure CRITICAL_SECTION is not referenced after deletion
	if (hUnloadDLLFunc)
		return hRes;

	while(!TryEnterCriticalSection(&d3d9EndMutex))
	{
		Sleep(0);

		if (hUnloadDLLFunc)
			return hRes;
	}

	d3d9EndScene.Unhook();

	hRes = device->EndScene();

	// Consecutive EndScene calls without Get/SetCursorPos pair
	if (consec_EndScene < 6)
		++consec_EndScene;

	d3d9EndScene.Rehook();

	LeaveCriticalSection(&d3d9EndMutex);

	return hRes;
}

typedef IDirect3D9* (WINAPI*D3D9CREATEPROC)(UINT);
typedef HRESULT (WINAPI*D3D9CREATEEXPROC)(UINT, IDirect3D9Ex**);

// Figure out the D3D9 device address and hook EndScene
bool InitD3D9Capture()
{
	bool bSuccess = false;

	// Get full path for the D3D9 DLL
	wchar_t lpD3D9Path[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, lpD3D9Path);
	size_t size = 11;
	wchar_t* wa = new wchar_t[size];
	mbstowcs_s(&size, wa, 11, TEXT("\\d3d9.dll"), 11);
	wcscat_s(lpD3D9Path, MAX_PATH, wa);
	delete[] wa;

	HMODULE hD3D9Dll = NULL;

	// Start creation of D3D9Ex device in dummy window
	if (hD3D9Dll = GetModuleHandleW(lpD3D9Path))
	{
		D3D9CREATEEXPROC d3d9CreateEx = (D3D9CREATEEXPROC)GetProcAddress(hD3D9Dll, "Direct3DCreate9Ex");

		if (d3d9CreateEx)
		{
			HRESULT hRes;

			IDirect3D9Ex *d3d9ex;

			if (SUCCEEDED(hRes = (*d3d9CreateEx)(D3D_SDK_VERSION, &d3d9ex)))
			{
				D3DPRESENT_PARAMETERS pp;
				ZeroMemory(&pp, sizeof(pp));
				pp.Windowed = 1;
				pp.SwapEffect = D3DSWAPEFFECT_FLIP;
				pp.BackBufferFormat = D3DFMT_A8R8G8B8;
				pp.BackBufferCount = 1;
				pp.hDeviceWindow = (HWND)hwndSender;
				pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

				IDirect3DDevice9Ex *deviceEx;

				if (SUCCEEDED(hRes = d3d9ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_NULLREF, hwndSender, D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_NOWINDOWCHANGES, &pp, NULL, &deviceEx)))
				{
					// D3D9Ex device made successfully in dummy window
					bSuccess = true;

					// D3D9Ex device address is the D3D9 vTable address
					UPARAM *vtable = *(UPARAM**)deviceEx;

					InitializeCriticalSection(&d3d9EndMutex);

					// EndScene address is a fixed offset in vTable
					d3d9EndScene.Hook((FARPROC)*(vtable+(168/4)), (FARPROC)D3D9EndScene);
					
					deviceEx->Release();

					d3d9EndScene.Rehook();
				}

				d3d9ex->Release();
			}
		}
	}
	
	return bSuccess;
}

// Releases remaining resources and frees the RInput DLL
DWORD WINAPI UnloadDLLFunc(LPVOID lpParameter)
{
	if (bD3D9Hooked)
	{
		EnterCriticalSection(&d3d9EndMutex);
		DeleteCriticalSection(&d3d9EndMutex);
		bD3D9Hooked = false;
	}

	CloseHandle(hUnloadDLLFunc);
	FreeLibraryAndExitThread(g_hInstance, 0);
	return 1;
}