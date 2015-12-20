/*
	This version of RInput was forked from abort's v1.31 by Vols and
	Jezuz (http://steamcommunity.com/profiles/76561198057348857/), with
	a lot of help from BuSheeZy (http://BushGaming.com) and qsxcv
	(http://www.overclock.net/u/395745/qsxcv).

	------------------------------------------------------------------
	Comments from original author, abort (http://blog.digitalise.net/)
	------------------------------------------------------------------

	RInput allows you to override low definition windows mouse input
	with high definition mouse input.

	RInput Copyright (C) 2012, J. Dijkstra (abort@digitalise.net)

	This file is part of RInput.

	RInput is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	RInput is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with RInput.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include "detours.h"
#include <commctrl.h>

#pragma intrinsic(memset)

// Initialize static variables
HWND CRawInput::hwndClient = NULL;
bool CRawInput::TF2unblock = false;
long CRawInput::hold_x = 0;
long CRawInput::hold_y = 0;
POINT CRawInput::centerPoint;
HWND CRawInput::hwndInput = NULL;
long CRawInput::set_x = 0;
long CRawInput::set_y = 0;
bool CRawInput::bRegistered = false;
CRITICAL_SECTION CRawInput::rawMouseData;
long CRawInput::x = 0;
long CRawInput::y = 0;
int CRawInput::signal = 0;
int CRawInput::consecG = 2;
bool CRawInput::alttab = false;
int CRawInput::SCP = 0;
bool CRawInput::bSubclass = false;
HANDLE CRawInput::hCreateThread = NULL;

// Define functions that are to be hooked and detoured
extern "C" DETOUR_TRAMPOLINE(int __stdcall TrmpGetCursorPos(LPPOINT lpPoint), GetCursorPos);
extern "C" DETOUR_TRAMPOLINE(int __stdcall TrmpSetCursorPos(int x, int y), SetCursorPos);

struct WindowHandleStructure
{
	unsigned long PID;
	HWND WindowHandle;
};

bool CRawInput::initialize(WCHAR* pwszError)
{
	if (!initWindow(pwszError))
		return false;

	if (!initInput(pwszError))
		return false;

	return true;
}

bool CRawInput::initWindow(WCHAR* pwszError)
{
	// Identify the window that matches the injected process
	CRawInput::hwndClient = CRawInput::clientWindow(GetCurrentProcessId());

	if (CRawInput::hwndClient)
	{
		// TF2 Window must be active for backpack fixes to work
		if (n_sourceEXE == TF2 && GetForegroundWindow() != CRawInput::hwndClient)
		{
			CRawInput::TF2unblock = true;
			BlockInput(TRUE);
			ShowWindow(CRawInput::hwndClient, 1);
		}

		// Set screen center until SetCursorPos is called
		if (CRawInput::clientCenter())
		{
			CRawInput::hold_x = CRawInput::centerPoint.x;
			CRawInput::hold_y = CRawInput::centerPoint.y;
		}

		// Unblock TF2 input after making it foreground
		if (CRawInput::TF2unblock)
		{
			Sleep(1000);
			BlockInput(FALSE);
		}
	}

	// Register the window to catch WM_INPUT events
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)wpInput;
	wcex.lpszClassName = "RInput";

	if (!RegisterClassEx(&wcex))
	{
		lstrcpyW(pwszError, L"Failed to register input window!");
		return false;
	}

	// Create the window to catch WM_INPUT events	
	CRawInput::hwndInput = CreateWindowEx(NULL, "RInput", "RInput", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

	if (!CRawInput::hwndInput) 
	{
		lstrcpyW(pwszError, L"Failed to create input window!");
		return false;
	}

	// Unregister the window class
	UnregisterClass("RInput", NULL);
	return true;
}

bool CRawInput::initInput(WCHAR* pwszError)
{
	POINT defCor;

	// Raw input accumulators initialized to starting cursor position
	if (GetCursorPos(&defCor))
	{
		CRawInput::set_x = defCor.x;
		CRawInput::set_y = defCor.y;
	}

	RAWINPUTDEVICE rMouse;
	memset(&rMouse, 0, sizeof(RAWINPUTDEVICE));

	// Flag allows accumulation to continue for TF2 while alt-tabbed
	if (n_sourceEXE == TF2)
		rMouse.dwFlags = RIDEV_INPUTSINK;
	else
		rMouse.dwFlags = 0;

	rMouse.hwndTarget = CRawInput::hwndInput;
	rMouse.usUsagePage = 0x01;
	rMouse.usUsage = 0x02;

	if (!RegisterRawInputDevices(&rMouse, 1, sizeof(RAWINPUTDEVICE)))
	{
		lstrcpyW(pwszError, L"Failed to register raw input device!");
		return false;
	}

	return (bRegistered = true);
}

BOOL CALLBACK CRawInput::EnumWindowsProc(HWND WindowHandle, LPARAM lParam)
{
	unsigned long PID = 0;
	WindowHandleStructure* data = reinterpret_cast<WindowHandleStructure*>(lParam);
	GetWindowThreadProcessId(WindowHandle, &PID);

	if (data->PID != PID || GetWindow(WindowHandle, GW_OWNER) || !IsWindowVisible(WindowHandle))
		return TRUE;

	// Found visible, main program window matching current process ID
	data->WindowHandle = WindowHandle;
	return FALSE;
}

HWND CRawInput::clientWindow(unsigned long PID)
{
	WindowHandleStructure data = {PID, NULL};
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&data));
	return data.WindowHandle;
}

bool CRawInput::clientCenter()
{
	RECT rectClient;

	if (GetClientRect(CRawInput::hwndClient, &rectClient))
	{
		// Calculated relative window center
		long xClient = rectClient.right / 2;
		long yClient = rectClient.bottom / 2;
		centerPoint.x = xClient;
		centerPoint.y = yClient;

		if (ClientToScreen(CRawInput::hwndClient, &centerPoint))
			// Translated relative window center to resolution coords
			return true;
	}
	
	return false;
}

unsigned int CRawInput::pollInput()
{
	MSG msg;

	while (GetMessage(&msg, CRawInput::hwndInput, 0, 0) != 0)
		DispatchMessage(&msg);

	return msg.message;
}

LRESULT __stdcall CRawInput::wpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INPUT:
			{
				UINT uiSize = 40;
				static unsigned char lpb[40];
				RAWINPUT* rwInput;

				if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &uiSize, sizeof(RAWINPUTHEADER)) != -1)
				{
					rwInput = (RAWINPUT*)lpb;

					if (!rwInput->header.dwType)
					{
						// Avoid collisions with hGetCursorPos
						EnterCriticalSection(&CRawInput::rawMouseData);

						// Accumulate cursor pos change from raw packets
						CRawInput::x += rwInput->data.mouse.lLastX;
						CRawInput::y += rwInput->data.mouse.lLastY;

						LeaveCriticalSection(&CRawInput::rawMouseData);
					}
				}

				break;
			}

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int __stdcall CRawInput::hSetCursorPos(int x, int y)
{
	if (!TrmpSetCursorPos(x, y))
		return 1;

	CRawInput::set_x = (long)x;
	CRawInput::set_y = (long)y;

	if (n_sourceEXE != NOBUGFIXES)
	{
		if (n_sourceEXE == TF2)
		{
			if (CRawInput::signal >= 1)
				++CRawInput::signal;

			// Bug fix for Steam overlay in TF2 backpack
			if (consec_EndScene == MAX_CONSEC_ENDSCENE && CRawInput::consecG == MAX_CONSECG && !CRawInput::alttab)
			{
				if (CRawInput::SCP == 0)
					CRawInput::SCP = -1;

				goto skipGreset;
			}

			CRawInput::consecG = 0;
		}

skipGreset:
		++CRawInput::SCP;

		// Alt-tab bug fix
		if (CRawInput::set_x == 0 && CRawInput::set_y == 0)
			CRawInput::alttab = true;

		// Console and buy menu bug fixes
		if (CRawInput::SCP == 1)
		{
			CRawInput::set_x -= CRawInput::x;
			CRawInput::set_y -= CRawInput::y;
		}
		else if (CRawInput::SCP == 2)
		{
			if (n_sourceEXE != TF2)
				CRawInput::SCP = 0;

			consec_EndScene = 0;

			CRawInput::alttab = false;

			CRawInput::hold_x = CRawInput::set_x;
			CRawInput::hold_y = CRawInput::set_y;
		}
	}

	return 0;
}

int __stdcall CRawInput::hGetCursorPos(LPPOINT lpPoint)
{
	// Avoid collisions with accumulation of raw input packets
	EnterCriticalSection(&CRawInput::rawMouseData);

	// Split off raw input handling to accumulate independently
	CRawInput::set_x += CRawInput::x;
	CRawInput::set_y += CRawInput::y;
	CRawInput::x = 0;
	CRawInput::y = 0;

	LeaveCriticalSection(&CRawInput::rawMouseData);

	if (n_sourceEXE == TF2)
	{
		if (CRawInput::signal >= 1)
			++CRawInput::signal;
		else if (CRawInput::bSubclass)
		{
			// TF2 backpack fix not applied when in actual game
			RemoveWindowSubclass(CRawInput::hwndClient, CRawInput::SubclassWndProc, 0);
			CRawInput::bSubclass = false;
		}

		if (CRawInput::consecG < MAX_CONSECG)
			++CRawInput::consecG;

		// Bug fix for cursor hitting side of screen in TF2 backpack
		if (CRawInput::consecG == MAX_CONSECG)
		{
			if (CRawInput::set_x >= CRawInput::hold_x * 2)
				CRawInput::set_x = CRawInput::hold_x * 2 - 1;
			else if (CRawInput::set_x < 0)
				CRawInput::set_x = 0;

			if (CRawInput::set_y >= CRawInput::hold_y * 2)
				CRawInput::set_y = CRawInput::hold_y * 2 - 1;
			else if (CRawInput::set_y < 0)
				CRawInput::set_y = 0;

			// Fix for subtle TF2 backpack bug from alt-tabbing
			if (!CRawInput::bSubclass)
			{
				if (!CRawInput::hwndClient)
					CRawInput::hwndClient = CRawInput::clientWindow(GetCurrentProcessId());

				if (CRawInput::hwndClient)
				{
					// When in TF2 backpack, monitor its window messages
					SetWindowSubclass(CRawInput::hwndClient, CRawInput::SubclassWndProc, 0, 1);
					CRawInput::bSubclass = true;
				}
			}
		}
	}

	if (!CRawInput::alttab)
	{
		if (consec_EndScene == MAX_CONSEC_ENDSCENE)
		{
			// Needed to not break backpack in TF2
			if (!(CRawInput::SCP != 1 && CRawInput::consecG == MAX_CONSECG))
			{
				if (!CRawInput::hwndClient)
					CRawInput::hwndClient = CRawInput::clientWindow(GetCurrentProcessId());

				// Buy and escape menu bug fixes
				if (CRawInput::hwndClient && CRawInput::clientCenter())
				{
					CRawInput::set_x = CRawInput::centerPoint.x;
					CRawInput::set_y = CRawInput::centerPoint.y;
				}
			}
		}

		lpPoint->x = CRawInput::set_x;
		lpPoint->y = CRawInput::set_y;
	}
	else
	{
		// Alt-tab bug fix
		lpPoint->x = CRawInput::hold_x;
		lpPoint->y = CRawInput::hold_y;
	}

	CRawInput::SCP = 0;
	return 0;
}

LRESULT CALLBACK CRawInput::SubclassWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
		case WM_SETFOCUS:
			// Bug fix during alt-tab back into TF2 backpack
			CRawInput::signal = 1;
			CRawInput::hCreateThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CRawInput::blockInput, NULL, 0, 0);
			break;

		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, CRawInput::SubclassWndProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI CRawInput::blockInput(LPVOID lpParameter)
{
	BlockInput(TRUE);

	// Unblock input after 9 SetCursorPos and/or GetCursorPos calls
	for (size_t q = 0; CRawInput::signal < 10; ++q)
		Sleep(16);

	CRawInput::signal = 0;

	BlockInput(FALSE);
	
	CloseHandle(CRawInput::hCreateThread);
	return 1;
}

bool CRawInput::hookLibrary(bool bInstall)
{
	if (bInstall)
	{
		if (!DetourFunctionWithTrampoline((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos) || !DetourFunctionWithTrampoline((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos))
			return false;

		InitializeCriticalSection(&CRawInput::rawMouseData);

		// Start CS:GO and TF2 D3D9 hooking
		if (n_sourceEXE <= 2)
			hD3D9HookThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)D3D9HookThread, NULL, 0, 0);
	}
	else 
	{
		DetourRemove((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos);
		DetourRemove((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos);

		DeleteCriticalSection(&CRawInput::rawMouseData);
	}

	return true;
}

void CRawInput::unload()
{
	if (bRegistered && CRawInput::hwndInput)
	{
		RAWINPUTDEVICE rMouse;
		memset(&rMouse, 0, sizeof(RAWINPUTDEVICE));
		rMouse.dwFlags |= RIDEV_REMOVE;
		rMouse.hwndTarget = NULL;
		rMouse.usUsagePage = 0x01;
		rMouse.usUsage = 0x02;
		RegisterRawInputDevices(&rMouse, 1, sizeof(RAWINPUTDEVICE));

		DestroyWindow(hwndInput);
	}
}