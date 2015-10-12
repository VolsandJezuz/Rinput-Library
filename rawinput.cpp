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

#include "rawinput.h"

// define the to be hooked functions
extern "C" DETOUR_TRAMPOLINE(int __stdcall TrmpGetCursorPos(LPPOINT lpPoint), GetCursorPos);
extern "C" DETOUR_TRAMPOLINE(int __stdcall TrmpSetCursorPos(int X, int Y), SetCursorPos);

// Initialize static variables
bool CRawInput::bRegistered = false;
HWND CRawInput::hwndInput = NULL;
long CRawInput::x = 0;
long CRawInput::y = 0;
long CRawInput::set_x = 0;
long CRawInput::set_y = 0;
bool CRawInput::s = false;

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
	// Register the window to catch WM_INPUT events
	WNDCLASSEX wcex;
	memset(&wcex, 0, sizeof(WNDCLASSEX));

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = (WNDPROC)wpInput;
	wcex.lpszClassName = INPUTWINDOW;

	if (!RegisterClassEx(&wcex))
	{
		lstrcpyW(pwszError, L"Failed to register input window!");
		return false;
	}

	// Create the window to catch WM_INPUT events	
	CRawInput::hwndInput = CreateWindowEx(NULL, INPUTWINDOW, INPUTWINDOW, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	if (!CRawInput::hwndInput) 
	{
		lstrcpyW(pwszError, L"Failed to create input window!");
		return false;
	}

#ifdef _DEBUG
	OutputDebugString("Created Raw Input Window");
#endif

	// Unregister the window class
	UnregisterClass(INPUTWINDOW, NULL);

	return true;
}

bool CRawInput::initInput(WCHAR* pwszError)
{
	// Set default coordinates
	CRawInput::x = CRawInput::y = CRawInput::set_x = CRawInput::set_y = 0;
	LPPOINT defCor = new tagPOINT;
	GetCursorPos(defCor);
	CRawInput::set_x = defCor->x;
	CRawInput::set_y = defCor->y;

	RAWINPUTDEVICE rMouse;
	memset(&rMouse, 0, sizeof(RAWINPUTDEVICE));

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
		case WM_INPUT:{
#ifdef _DEBUG
			OutputDebugString("WM_INPUT event");
#endif
			UINT uiSize = RAWPTRSIZE;
			static unsigned char lpb[RAWPTRSIZE];
			RAWINPUT* rwInput;

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &uiSize, RAWINPUTHDRSIZE) != -1)
			{
				rwInput = (RAWINPUT*)lpb;

				if (!rwInput->header.dwType)
				{
					CRawInput::x += rwInput->data.mouse.lLastX;
					CRawInput::y += rwInput->data.mouse.lLastY;
				}
			}
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default: return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int __stdcall CRawInput::hSetCursorPos(int x, int y)
{
	if (!TrmpSetCursorPos(x, y)) return 1;

	CRawInput::set_x = (long)x;
	CRawInput::set_y = (long)y;
	CRawInput::s = true;

#ifdef _DEBUG
	OutputDebugString("Set coordinates");
#endif

	return 0;
}

int __stdcall CRawInput::hGetCursorPos(LPPOINT lpPoint)
{

	lpPoint->x = CRawInput::set_x + CRawInput::x;
	lpPoint->y = CRawInput::set_y + CRawInput::y;

	if (CRawInput::s)
	{
		CRawInput::x = 0;
		CRawInput::y = 0;
		CRawInput::s = false;
	}

#ifdef _DEBUG
	OutputDebugString("Returned coordinates");
#endif

	return 0;
}

bool CRawInput::hookLibrary(bool bInstall)
{
	if (bInstall)
	{
		if (!DetourFunctionWithTrampoline((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos) || !DetourFunctionWithTrampoline((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos))
			return false;
#ifdef _DEBUG
		else
			OutputDebugString("Hooked GetCursorPos and SetCursorPos");
#endif
	}
	else 
	{
		if (DetourRemove((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos))
		{
#ifdef _DEBUG
			OutputDebugString("Removed GetCursorPos hook");
#endif
		}
		if (DetourRemove((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos))
		{
#ifdef _DEBUG
			OutputDebugString("Removed SetCursorPos hook");
#endif
		}
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

#ifdef _DEBUG
		OutputDebugString("Unregistered mouse device");
		OutputDebugString("Closed Input Window");
#endif
	}
}