/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and are only called for CS:GO.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"
#include <d3d9.h>

// EndScene is hooked to detect frame renders
HookData d3d9EndScene;

extern int consec_endscene;
extern CRITICAL_SECTION d3d9EndMutex;
extern bool bD3D9Hooked;

LPVOID lpCurrentDevice = NULL;
BOOL bD3D9Ex = FALSE;
HMODULE hD3D9Dll = NULL;

void SetupD3D9(IDirect3DDevice9 *device);

typedef HRESULT(STDMETHODCALLTYPE *D3D9EndScenePROC)(IDirect3DDevice9 *device);

HRESULT STDMETHODCALLTYPE D3D9EndScene(IDirect3DDevice9 *device)
{
    EnterCriticalSection(&d3d9EndMutex);

    d3d9EndScene.Unhook();

    if(lpCurrentDevice == NULL)
    {
        IDirect3D9 *d3d;
        if(SUCCEEDED(device->GetDirect3D(&d3d)))
        {
            IDirect3D9 *d3d9ex;
            if(bD3D9Ex = SUCCEEDED(d3d->QueryInterface(__uuidof(IDirect3D9Ex), (void**)&d3d9ex)))
                d3d9ex->Release();
            d3d->Release();
        }

        if(!bTargetAcquired)
        {
            lpCurrentDevice = device;
            SetupD3D9(device);
            bTargetAcquired = true;
        }
    }

	// EndScene is called twice per frame render
	if (consec_endscene < 5)
		++consec_endscene;

    HRESULT hRes = device->EndScene();
    d3d9EndScene.Rehook();

    LeaveCriticalSection(&d3d9EndMutex);

    return hRes;
}

void LogPresentParams(D3DPRESENT_PARAMETERS &pp);

void SetupD3D9(IDirect3DDevice9 *device)
{
	IDirect3D9 *d3d;
	if (SUCCEEDED(device->GetDirect3D(&d3d)))
	{
		IDirect3D9 *d3d9ex;
		if (bD3D9Ex = SUCCEEDED(d3d->QueryInterface(__uuidof(IDirect3D9Ex), (void**)&d3d9ex)))
			d3d9ex->Release();
		d3d->Release();
	}
}

typedef IDirect3D9* (WINAPI*D3D9CREATEPROC)(UINT);
typedef HRESULT (WINAPI*D3D9CREATEEXPROC)(UINT, IDirect3D9Ex**);

bool InitD3D9Capture()
{
	bool bSuccess = false;

	wchar_t lpD3D9Path[MAX_PATH];
	SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, lpD3D9Path);
	size_t size = 11;
    wchar_t* wa = new wchar_t[size];
    mbstowcs_s(&size, wa, 11, TEXT("\\d3d9.dll"), 11);
	wcscat_s(lpD3D9Path, MAX_PATH, wa);

	hD3D9Dll = GetModuleHandleW(lpD3D9Path);
	if (hD3D9Dll)
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
					bSuccess = true;

					UPARAM *vtable = *(UPARAM**)deviceEx;

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

void CheckD3D9Capture()
{
	EnterCriticalSection(&d3d9EndMutex);
	d3d9EndScene.Rehook(true);
	LeaveCriticalSection(&d3d9EndMutex);
}