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

#include "main.h"
#include <shlwapi.h>

int n_sourceEXE = 0;
bool sourceEXE = false;
HANDLE hCaptureThread = NULL;
bool bCaptureThreadStop = false;

int __stdcall DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);

	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			// No need for a threaded entry
			if (!DisableThreadLibraryCalls(hInstance))
				return 0;

			{ // Get process and enable bug fixes for source games
				char szEXEPath[MAX_PATH];
				GetModuleFileNameA(NULL, szEXEPath, sizeof(szEXEPath));
				PathStripPathA(szEXEPath);

				// Bug fixes now limited to source games that have been tested
				char *source_exes[] = {"csgo.exe", "hl2.exe", "portal2.exe", NULL};

				for (char **iSource_exes = source_exes; *iSource_exes != NULL; ++iSource_exes)
				{
					++n_sourceEXE;

					if ((std::string)szEXEPath == (std::string)*iSource_exes)
					{
						// Start D3D hooking for CS:GO
						if (n_sourceEXE == 1)
						{
							HANDLE hThread = NULL;
							HANDLE hDllMainThread = OpenThread(THREAD_ALL_ACCESS, NULL, GetCurrentThreadId());

							if (!(hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CaptureThread, (LPVOID)hDllMainThread, 0, 0)))
							{
								CloseHandle(hDllMainThread);
								return 0;
							}

							hCaptureThread = hThread;
							CloseHandle(hThread);
						}

						sourceEXE = true;

						break;
					}
				}
			}

			g_hInstance = hInstance;

			break;

		case DLL_PROCESS_DETACH:
			if (n_sourceEXE == 1) // Stop D3D hooking for CS:GO
			{
				DeleteCriticalSection(&d3d9EndMutex);

				bCaptureThreadStop = true;
				WaitForSingleObject(hCaptureThread, 300);

				if (dummyEvent)
					CloseHandle(dummyEvent);

				if (hwndSender)
					DestroyWindow(hwndSender);

				if (hCaptureThread)
					CloseHandle(hCaptureThread);
			}

			CRawInput::hookLibrary(false);
			CRawInput::unload();

			break;
	}

	return 1;
}

extern "C" __declspec(dllexport) void entryPoint()
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, EVENTNAME);
	if (!hEvent)
		displayError(L"Could not open interprocess communication event.");

	WCHAR pwszError[ERROR_BUFFER_SIZE];

	if (!validateVersion())
		displayError(L"You must at least have Microsoft Windows XP to use this library.");

	if (!CRawInput::initialize(pwszError))
		displayError(pwszError);

	if (!CRawInput::hookLibrary(true))
		displayError(L"Failed to hook Windows API cursor functions.");

	// Signal the injector that the injection and hooking are successful
	if (!SetEvent(hEvent))
		displayError(L"Failed to signal the initialization event.");

	CloseHandle(hEvent);

	if (!CRawInput::pollInput())
		displayError(L"Failed to poll mouse input");
}

// Validate that we are working with at least Windows XP
inline bool validateVersion()
{
	DWORD dwVersion = GetVersion();
	double fCompareVersion = LOBYTE(LOWORD(dwVersion)) + 0.1 * HIBYTE(LOWORD(dwVersion));
	return (dwVersion && fCompareVersion >= 5.1);
}

void displayError(WCHAR* pwszError)
{
	MessageBoxW(NULL, pwszError, L"Raw Input error!", MB_ICONERROR | MB_OK);

	CRawInput::hookLibrary(false);

	unloadLibrary();
}

void unloadLibrary()
{
	__asm
	{
		push -2
		push 0
		push g_hInstance
		mov eax, TerminateThread
		push eax
		mov eax, FreeLibrary
		jmp eax
	}
}