/********************************************************************************
 hook.h, hook.cpp, and d3d9.cpp contain the code for fixing the bugs in
 the CS:GO buy and escape menus, and are only called for CS:GO.

 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This new code was adapted from the Open Broadcaster Software source,
 obtained from https://github.com/jp9000/OBS
********************************************************************************/

#include "hook.h"
#include <d3d9.h>

// Prevent empty controlled statement warning, as it's intentional
#pragma warning(disable: 4390)

typedef HRESULT (WINAPI *PRESENTPROC)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
typedef HRESULT (WINAPI *PRESENTEXPROC)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);
typedef HRESULT (WINAPI *SWAPPRESENTPROC)(IDirect3DSwapChain9*, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);

HookData d3d9EndScene;
HookData d3d9Reset;
HookData d3d9ResetEx;
HookData d3d9Present;
HookData d3d9PresentEx;
HookData d3d9SwapPresent;

extern int frame_rendered;
extern CRITICAL_SECTION d3d9EndMutex;
extern bool bD3D9Hooked;

LPVOID lpCurrentDevice = NULL;
BOOL bD3D9Ex = FALSE;
HMODULE hD3D9Dll = NULL;

void SetupD3D9(IDirect3DDevice9 *device);

static int presentRecurse = 0;

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

	HRESULT hRes = device->EndScene();
	d3d9EndScene.Rehook();

	LeaveCriticalSection(&d3d9EndMutex);

	return hRes;
}

HRESULT STDMETHODCALLTYPE D3D9Present(IDirect3DDevice9 *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion)
{
	d3d9Present.Unhook();

	if (!presentRecurse)
	{
		if (frame_rendered < 3)
			++frame_rendered;
	}
	presentRecurse++;
	HRESULT hRes = device->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
	presentRecurse--;
	
	d3d9Present.Rehook();
	
	return hRes;
}

HRESULT STDMETHODCALLTYPE D3D9PresentEx(IDirect3DDevice9Ex *device, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	d3d9PresentEx.Unhook();

	if (!presentRecurse)
	{
		if (frame_rendered < 3)
			++frame_rendered;
	}
	presentRecurse++;
	HRESULT hRes = device->PresentEx(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
	presentRecurse--;

	d3d9PresentEx.Rehook();

	return hRes;
}

HRESULT STDMETHODCALLTYPE D3D9SwapPresent(IDirect3DSwapChain9 *swap, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion, DWORD dwFlags)
{
	d3d9SwapPresent.Unhook();

	if(!presentRecurse)
	{
		if (frame_rendered < 3)
			++frame_rendered;
	}
	presentRecurse++;
	HRESULT hRes = swap->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
	presentRecurse--;

	d3d9SwapPresent.Rehook();

	return hRes;
}

typedef HRESULT(STDMETHODCALLTYPE *D3D9ResetPROC)(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *params);

HRESULT STDMETHODCALLTYPE D3D9Reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *params)
{
	d3d9Reset.Unhook();

	HRESULT hRes = device->Reset(params);

	if(lpCurrentDevice == NULL && !bTargetAcquired)
	{
		lpCurrentDevice = device;
		bTargetAcquired = true;
	}

	if(lpCurrentDevice == device)
		SetupD3D9(device);

	d3d9Reset.Rehook();

	return hRes;
}

HRESULT STDMETHODCALLTYPE D3D9ResetEx(IDirect3DDevice9Ex *device, D3DPRESENT_PARAMETERS *params, D3DDISPLAYMODEEX *fullscreenData)
{
	d3d9ResetEx.Unhook();
	d3d9Reset.Unhook();

	HRESULT hRes = device->ResetEx(params, fullscreenData);

	if(lpCurrentDevice == NULL && !bTargetAcquired)
	{
		lpCurrentDevice = device;
		bTargetAcquired = true;
		bD3D9Ex = true;
	}

	if(lpCurrentDevice == device)
		SetupD3D9(device);

	d3d9Reset.Rehook();
	d3d9ResetEx.Rehook();

	return hRes;
}

void SetupD3D9(IDirect3DDevice9 *device)
{
	IDirect3DSwapChain9 *swapChain = NULL;
	if (SUCCEEDED(device->GetSwapChain(0, &swapChain)))
	{
		D3DPRESENT_PARAMETERS pp;
		if (SUCCEEDED(swapChain->GetPresentParameters(&pp)))
			; //intentionally empty

		IDirect3D9 *d3d;
		if (SUCCEEDED(device->GetDirect3D(&d3d)))
		{
			IDirect3D9 *d3d9ex;
			if (bD3D9Ex = SUCCEEDED(d3d->QueryInterface(__uuidof(IDirect3D9Ex), (void**)&d3d9ex)))
				d3d9ex->Release();
			d3d->Release();
		}

		d3d9Present.Hook(GetVTable(device, (68/4)), (FARPROC)D3D9Present);
		if(bD3D9Ex)
		{
			FARPROC curPresentEx = GetVTable(device, (484/4));
			d3d9PresentEx.Hook(curPresentEx, (FARPROC)D3D9PresentEx);
			d3d9ResetEx.Hook(GetVTable(device, (528/4)), (FARPROC)D3D9ResetEx);
		}
		d3d9Reset.Hook(GetVTable(device, (64/4)), (FARPROC)D3D9Reset);
		d3d9SwapPresent.Hook(GetVTable(swapChain, (12/4)), (FARPROC)D3D9SwapPresent);
		d3d9Present.Rehook();
		d3d9SwapPresent.Rehook();
		d3d9Reset.Rehook();
        if(bD3D9Ex) {
            d3d9PresentEx.Rehook();
            d3d9ResetEx.Rehook();
        }

		swapChain->Release();
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
	delete[] wa;

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