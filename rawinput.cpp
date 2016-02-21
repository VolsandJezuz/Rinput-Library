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

#pragma intrinsic(memset)

// Define functions that are to be hooked and detoured
extern "C" DETOUR_TRAMPOLINE(BOOL WINAPI TrmpGetCursorPos(LPPOINT lpPoint), GetCursorPos);
extern "C" DETOUR_TRAMPOLINE(BOOL WINAPI TrmpSetCursorPos(int x, int y), SetCursorPos);

unsigned char CRawInput::n_sourceEXE = 1;
HWND CRawInput::hwndClient = NULL;
bool CRawInput::TF2unblock = false;
POINT CRawInput::centerPoint;
long CRawInput::leftBoundary = 0;
long CRawInput::rightBoundary = 0;
long CRawInput::topBoundary = 0;
long CRawInput::bottomBoundary = 0;
long CRawInput::hold_x = 0;
long CRawInput::hold_y = 0;
CRITICAL_SECTION CRawInput::rawMouseData;
long CRawInput::x = 0;
long CRawInput::y = 0;
HWND CRawInput::hwndInput = NULL;
long CRawInput::set_x = 0;
long CRawInput::set_y = 0;
bool CRawInput::bRegistered = false;
unsigned char CRawInput::signal = 0;
bool CRawInput::bSubclass = false;
HANDLE CRawInput::hCreateThread = NULL;
unsigned char CRawInput::consecG = 2;
bool CRawInput::alttab = false;
unsigned char CRawInput::consec_EndScene = 0;
char CRawInput::SCP = 0;
HANDLE CRawInput::hD3D9HookThread = NULL;
DWORD CRawInput::oD3D9EndScene = 0;

bool CRawInput::initialize(WCHAR* pwszError)
{
	wchar_t szEXEPath[MAX_PATH];

	// Detect source games for enabling specific bug fixes
	if (GetModuleFileNameW(NULL, szEXEPath, sizeof(szEXEPath)))
	{
		wchar_t TF2path[sizeof(szEXEPath)];
		wcscpy_s(TF2path, sizeof(TF2path), szEXEPath);
		PathStripPathW(szEXEPath);
		wchar_t *source_exes[] = {L"csgo.exe", L"hl2.exe", L"portal2.exe", NULL};
				
		// Bug fixes now limited to tested source games
		while (source_exes[CRawInput::n_sourceEXE - 1] != NULL && wcscmp(szEXEPath, source_exes[CRawInput::n_sourceEXE - 1]))
			++CRawInput::n_sourceEXE;
		if (CRawInput::n_sourceEXE == TF2)
		{
			PathRemoveFileSpecW(TF2path);
			wchar_t tf2[16] = L"Team Fortress 2";

			// Make sure hl2.exe is TF2
			for (size_t k = 0; k < 15; ++k)
			{
				if (TF2path[((std::wstring)TF2path).size() + k - 15] != tf2[k])
				{
					// Treat other hl2 games like portal and exit loop
					++CRawInput::n_sourceEXE;
					k += 15 - k;
				}
			}
		}
	}
	else
		CRawInput::n_sourceEXE = NO_BUG_FIXES;

	// Avoid collisions with accumulation of raw input packets
	InitializeCriticalSection(&CRawInput::rawMouseData);

	if (!CRawInput::initWindow(pwszError))
		return false;

	if (!CRawInput::initInput(pwszError))
		return false;

	return true;
}

bool CRawInput::initWindow(WCHAR* pwszError)
{
	// Identify the window that matches the injected process
	EnumWindows(CRawInput::EnumWindowsProc, (LPARAM)GetCurrentProcessId());

	if (CRawInput::hwndClient)
	{
		// TF2 Window must be active for backpack fixes to work
		if (CRawInput::n_sourceEXE == TF2 && GetForegroundWindow() != CRawInput::hwndClient)
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
	wcex.lpfnWndProc = (WNDPROC)CRawInput::wpInput;
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

BOOL CALLBACK CRawInput::EnumWindowsProc(HWND WindowHandle, LPARAM lParam)
{
	unsigned long PID = 0;
	GetWindowThreadProcessId(WindowHandle, &PID);

	if (PID != (unsigned long)lParam || (GetWindow(WindowHandle, GW_OWNER) && !IsWindowVisible(WindowHandle)))
		return TRUE;

	// Found visible, main program window matching current process ID
	CRawInput::hwndClient = WindowHandle;
	return FALSE;
}

bool CRawInput::clientCenter()
{
	RECT rectClient;

	// Try to get relative window center
	if (GetClientRect(CRawInput::hwndClient, &rectClient))
	{
		CRawInput::centerPoint.x = (long)((unsigned long)rectClient.right / 2);
		CRawInput::centerPoint.y = (long)((unsigned long)rectClient.bottom / 2);

		// Try to translate relative window center to absolute coords
		if (ClientToScreen(CRawInput::hwndClient, &CRawInput::centerPoint))
		{
			// Get absolute coords of game display monitor for TF2
			if (CRawInput::n_sourceEXE == TF2)
			{
				CRawInput::leftBoundary = (long)GetSystemMetrics(SM_XVIRTUALSCREEN);
				CRawInput::rightBoundary = (long)(GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN));
				CRawInput::topBoundary = (long)GetSystemMetrics(SM_YVIRTUALSCREEN);
				CRawInput::bottomBoundary = (long)(GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN));
			}

			return true;
		}
	}
	
	return false;
}

LRESULT CALLBACK CRawInput::wpInput(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INPUT:
			{
				UINT uiSize = 40;
				static BYTE lpb[40];

				if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &uiSize, sizeof(RAWINPUTHEADER)) != -1)
				{
					RAWINPUT* rwInput = (RAWINPUT*)lpb;

					if (!rwInput->header.dwType)
					{
						// Accumulate cursor position change
						EnterCriticalSection(&CRawInput::rawMouseData);
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
	if (CRawInput::n_sourceEXE == TF2)
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

	return (CRawInput::bRegistered = true);
}

bool CRawInput::hookLibrary(bool bInstall)
{
	if (bInstall)
	{
		if (!DetourFunctionWithTrampoline((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos) || !DetourFunctionWithTrampoline((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos))
			return false;

		// Start CS:GO and TF2 D3D9 hooking
		if (CRawInput::n_sourceEXE <= 2)
			CRawInput::hD3D9HookThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CRawInput::D3D9HookThread, NULL, 0, 0);
	}
	else 
	{
		DetourRemove((PBYTE)TrmpGetCursorPos, (PBYTE)CRawInput::hGetCursorPos);
		DetourRemove((PBYTE)TrmpSetCursorPos, (PBYTE)CRawInput::hSetCursorPos);
		DeleteCriticalSection(&CRawInput::rawMouseData);

		if (CRawInput::hD3D9HookThread)
			CloseHandle(CRawInput::hD3D9HookThread);
	}

	return true;
}

BOOL WINAPI CRawInput::hGetCursorPos(LPPOINT lpPoint)
{
	// Split off raw input handling to accumulate independently
	EnterCriticalSection(&CRawInput::rawMouseData);
	CRawInput::set_x += CRawInput::x;
	CRawInput::set_y += CRawInput::y;
	CRawInput::x = CRawInput::y = 0;
	LeaveCriticalSection(&CRawInput::rawMouseData);

	if (CRawInput::n_sourceEXE == TF2)
	{
		if (CRawInput::signal >= 1)
			++CRawInput::signal;
		else if (CRawInput::bSubclass)
		{
			// TF2 backpack fix not applied when in actual game
			RemoveWindowSubclass(CRawInput::hwndClient, (SUBCLASSPROC)CRawInput::SubclassWndProc, 0);
			CRawInput::bSubclass = false;
		}

		if (CRawInput::consecG < MAX_CONSECG)
			++CRawInput::consecG;

		// Bug fix for cursor hitting side of screen in TF2 backpack
		if (CRawInput::consecG == MAX_CONSECG)
		{
				if (CRawInput::set_x < CRawInput::leftBoundary)
					CRawInput::set_x = CRawInput::leftBoundary;
				else if (CRawInput::set_x >= CRawInput::rightBoundary)
					CRawInput::set_x = CRawInput::rightBoundary - 1;

				if (CRawInput::set_y < CRawInput::topBoundary)
					CRawInput::set_y = CRawInput::topBoundary;
				else if (CRawInput::set_y >= CRawInput::bottomBoundary)
					CRawInput::set_y = CRawInput::bottomBoundary - 1;

			// Fix for subtle TF2 backpack bug from alt-tabbing
			if (!CRawInput::bSubclass)
			{
				if (!CRawInput::hwndClient)
					EnumWindows(CRawInput::EnumWindowsProc, (LPARAM)GetCurrentProcessId());

				if (CRawInput::hwndClient)
				{
					// When in TF2 backpack, monitor its window messages
					SetWindowSubclass(CRawInput::hwndClient, (SUBCLASSPROC)CRawInput::SubclassWndProc, 0, 1);
					CRawInput::bSubclass = true;
				}
			}
		}
	}

	if (!CRawInput::alttab)
	{
		// Buy and escape menu bug fixes
		if (CRawInput::consec_EndScene == MAX_CONSEC_ENDSCENE)
		{
			if (!CRawInput::hwndClient)
				EnumWindows(CRawInput::EnumWindowsProc, (LPARAM)GetCurrentProcessId());

			if (CRawInput::hwndClient && CRawInput::clientCenter())
			{
				CRawInput::hold_x = CRawInput::centerPoint.x;
				CRawInput::hold_y = CRawInput::centerPoint.y;
			}

			// Needed to not break backpack in TF2
			if (!(CRawInput::SCP != 1 && CRawInput::consecG == MAX_CONSECG))
			{
				CRawInput::set_x = CRawInput::hold_x;
				CRawInput::set_y = CRawInput::hold_y;
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
	return FALSE;
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
			RemoveWindowSubclass(hWnd, (SUBCLASSPROC)CRawInput::SubclassWndProc, uIdSubclass);
			break;

		default:
			break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI CRawInput::blockInput(LPVOID lpParameter)
{
	BlockInput(TRUE);
	unsigned char SleepTimer = 0;

	// Unblock input after 11 SetCursorPos/GetCursorPos or 2 seconds
	while (CRawInput::signal <= 12 && SleepTimer < 67)
	{
		++SleepTimer;
		Sleep(30);
	}

	CRawInput::signal = 0;
	BlockInput(FALSE);	
	CloseHandle(CRawInput::hCreateThread);
	return 1;
}

BOOL WINAPI CRawInput::hSetCursorPos(int x, int y)
{
	if (!TrmpSetCursorPos(x, y))
		return TRUE;

	CRawInput::set_x = (long)x;
	CRawInput::set_y = (long)y;

	if (CRawInput::n_sourceEXE != NO_BUG_FIXES)
	{
		if (CRawInput::n_sourceEXE == TF2)
		{
			if (CRawInput::signal >= 1)
				++CRawInput::signal;

			// Bug fix for Steam overlay in TF2 backpack
			if (CRawInput::consec_EndScene == MAX_CONSEC_ENDSCENE && CRawInput::consecG == MAX_CONSECG && !CRawInput::alttab && CRawInput::SCP == 0)
				--CRawInput::SCP;
			else
				CRawInput::consecG = 0;
		}

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
			if (CRawInput::n_sourceEXE != TF2)
				CRawInput::SCP = 0;

			CRawInput::consec_EndScene = 0;

			CRawInput::alttab = false;

			if (CRawInput::hold_x != CRawInput::set_x || CRawInput::hold_y != CRawInput::set_y)
			{
				if (!CRawInput::hwndClient)
					EnumWindows(CRawInput::EnumWindowsProc, (LPARAM)GetCurrentProcessId());

				if (CRawInput::hwndClient && CRawInput::clientCenter())
				{
					CRawInput::hold_x = CRawInput::centerPoint.x;
					CRawInput::hold_y = CRawInput::centerPoint.y;
				}
				else
				{
					CRawInput::hold_x = CRawInput::set_x;
					CRawInput::hold_y = CRawInput::set_y;
				}
			}
		}
	}

	return FALSE;
}

DWORD WINAPI CRawInput::D3D9HookThread(LPVOID lpParameter)
{
	HMODULE hD3D9Dll = NULL;

	while (!hD3D9Dll)
	{
		hD3D9Dll = GetModuleHandleA("d3d9.dll");
		Sleep(150);
	}

	DWORD D3D9Base = (DWORD)hD3D9Dll;
	DWORD oldEndScene = 0;
	DWORD dwVersion = GetVersion();
	double fCompareVersion = LOBYTE(LOWORD(dwVersion)) + 0.1 * HIBYTE(LOWORD(dwVersion));

	// Windows versions have different EndScene memory patterns
	if (fCompareVersion >= 6.2)
	{
		// Windows 8, Windows 8.1, and Windows 10
		for (DWORD g = 0; g < 0x128000; ++g)
		{
			++D3D9Base;

			if ((*(WORD*)(D3D9Base + 0x00)) == 0xFF8B && (*(WORD*)(D3D9Base + 0x02)) == 0x8B55 && (*(WORD*)(D3D9Base + 0x04)) == 0xFFEC && (*(WORD*)(D3D9Base + 0x06)) == 0x0875 && (*(WORD*)(D3D9Base + 0x08)) == 0x018B && (*(WORD*)(D3D9Base + 0x0A)) == 0x3E6A && (*(WORD*)(D3D9Base + 0x0C)) == 0x90FF)
			{
				oldEndScene = D3D9Base;
				break;
			}
		}
	}
	else if (fCompareVersion >= 6.0)
	{
		// Windows Vista and Windows 7
		for (DWORD g = 0; g < 0x128000; ++g)
		{
			++D3D9Base;

			if ((*(WORD*)(D3D9Base + 0x00)) == 0xFF8B && (*(WORD*)(D3D9Base + 0x02)) == 0x8B55 && (*(WORD*)(D3D9Base + 0x04)) == 0x8BEC && (*(WORD*)(D3D9Base + 0x06)) == 0x0855 && (*(WORD*)(D3D9Base + 0x08)) == 0x018B && (*(WORD*)(D3D9Base + 0x0A)) == 0x808B && (*(WORD*)(D3D9Base + 0x0C)) == 0x00F4 && (*(WORD*)(D3D9Base + 0x0E)) == 0x0000 && (*(WORD*)(D3D9Base + 0x10)) == 0x6A52 && (*(WORD*)(D3D9Base + 0x12)) == 0xFF3E && (*(WORD*)(D3D9Base + 0x14)) == 0x5DD0 && (*(WORD*)(D3D9Base + 0x16)) == 0x04C2)
			{
				oldEndScene = D3D9Base;
				break;
			}
		}
	}
	else if (fCompareVersion >= 5.1)
	{
		// Windows XP
		for (DWORD g = 0; g < 0x128000; ++g)
		{
			++D3D9Base;

			if ((*(WORD*)(D3D9Base + 0x00)) == 0xFF8B && (*(WORD*)(D3D9Base + 0x02)) == 0x8B55 && (*(WORD*)(D3D9Base + 0x04)) == 0x8BEC && (*(WORD*)(D3D9Base + 0x06)) == 0x0855 && (*(WORD*)(D3D9Base + 0x08)) == 0x018B && (*(WORD*)(D3D9Base + 0x0A)) == 0x6A52)
			{
				oldEndScene = D3D9Base;
				break;
			}
		}
	}
	
	if (oldEndScene)
	{
		CRawInput::oD3D9EndScene = oldEndScene + 5;
		CRawInput::JMPplace((PBYTE)oldEndScene, (DWORD)CRawInput::D3D9EndScene, 5);
	}

	CloseHandle(CRawInput::hD3D9HookThread);
	return 1;
}

// Midhook avoids access violations if D3D9 is hooked by another program
void CRawInput::JMPplace(PBYTE inFunc, DWORD deFunc, DWORD len)
{
	DWORD oldPro = 0;
	VirtualProtect((LPVOID)inFunc, (SIZE_T)len, PAGE_EXECUTE_READWRITE, &oldPro);
	DWORD relAddress = 0;
	relAddress = (deFunc - (DWORD)inFunc) - 5;
	*inFunc = 0xE9;
	*((DWORD *)(inFunc + 0x1)) = relAddress;
	
	for (DWORD x = 0x5; x < len; ++x)
		*(inFunc + x) = 0x90;
	
	DWORD newPro = 0;
	VirtualProtect((LPVOID)inFunc, (SIZE_T)len, oldPro, &newPro);
}

__declspec(naked) HRESULT WINAPI CRawInput::D3D9EndScene()
{
	static LPDIRECT3DDEVICE9 pDevice;

	__asm
	{
		mov edi, edi
		push ebp
		mov ebp, esp
		mov eax, dword ptr ss:[ebp + 0x8]
		mov pDevice, eax
		pushad
	}

	if (pDevice && CRawInput::consec_EndScene < MAX_CONSEC_ENDSCENE)
		++CRawInput::consec_EndScene;

	__asm
	{
		popad
		jmp[CRawInput::oD3D9EndScene]
	}
}

UINT CRawInput::pollInput()
{
	MSG msg;

	while (GetMessage(&msg, CRawInput::hwndInput, 0, 0) != 0)
		DispatchMessage(&msg);

	return msg.message;
}

void CRawInput::unload()
{
	if (CRawInput::bRegistered && CRawInput::hwndInput)
	{
		RAWINPUTDEVICE rMouse;
		memset(&rMouse, 0, sizeof(RAWINPUTDEVICE));
		rMouse.dwFlags |= RIDEV_REMOVE;
		rMouse.hwndTarget = NULL;
		rMouse.usUsagePage = 0x01;
		rMouse.usUsage = 0x02;
		RegisterRawInputDevices(&rMouse, 1, sizeof(RAWINPUTDEVICE));
		DestroyWindow(CRawInput::hwndInput);
	}
}