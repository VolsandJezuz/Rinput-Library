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
#include <string>
#include <shlwapi.h>

static HINSTANCE g_hInstance = NULL;

// Expose the entry point function called by RInput.exe
extern "C" __declspec(dllexport) void entryPoint();

int __stdcall DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch(dwReason)
	{
		case DLL_PROCESS_ATTACH:
			// No need for a threaded entry
			if (!DisableThreadLibraryCalls(hInstance))
				return 0;
			else
			{
				g_hInstance = hInstance;
				char szEXEPath[MAX_PATH];

				// Detect source games for enabling specific bug fixes
				if (GetModuleFileNameA(NULL, szEXEPath, sizeof(szEXEPath)))
				{
					char TF2path[sizeof(szEXEPath)];
					strcpy_s(TF2path, _countof(TF2path), szEXEPath);
					PathStripPathA(szEXEPath);
					// Bug fixes now limited to tested source games
					char *source_exes[] = {"csgo.exe", "hl2.exe", "portal2.exe", NULL};

					for (char **iSource_exes = source_exes; *iSource_exes != NULL; ++iSource_exes)
					{
						++n_sourceEXE;

						if ((std::string)szEXEPath == (std::string)*iSource_exes)
						{
							if (n_sourceEXE == TF2)
							{
								// Make sure hl2.exe is TF2
								PathRemoveFileSpecA(TF2path);
								std::string sTF2path = (std::string)TF2path;
								char testTF2[16];

								for (size_t j = 1; j <= 15; ++j)
									testTF2[j] = TF2path[sTF2path.size() + j - 15];

								char tf2[16] = "Team Fortress 2";

								for (int k = 1; k < 16; ++k)
								{
									// Check hl2.exe is TF2
									if (testTF2[k] != tf2[k])
										n_sourceEXE = NOBUGFIXES;
								}
							}

							break;
						}
					}
				}
				else
					n_sourceEXE = NOBUGFIXES;
			}

			break;

		case DLL_PROCESS_DETACH:
			// Stop CS:GO and TF2 D3D9 hooking
			CRawInput::hookLibrary(false);
			CRawInput::unload();
			break;
	}

	return 1;
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

void displayError(WCHAR* pwszError)
{
	MessageBoxW(NULL, pwszError, L"Raw Input error!", MB_ICONERROR | MB_OK);
	CRawInput::hookLibrary(false);
	unloadLibrary();
}

// Validate that we are working with at least Windows XP
inline bool validateVersion()
{
	DWORD dwVersion = GetVersion();
	double fCompareVersion = LOBYTE(LOWORD(dwVersion)) + 0.1 * HIBYTE(LOWORD(dwVersion));
	return (dwVersion && fCompareVersion >= 5.1);
}

extern "C" __declspec(dllexport) void entryPoint()
{
	HANDLE hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, "RInputEvent32");

	if (!hEvent)
		displayError(L"Could not open interprocess communication event.");

	WCHAR pwszError[256];

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