// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>
#include <userenv.h>


//	Process startup info
static STARTUPINFO si;
static PROCESS_INFORMATION	pi;
static const char *ERREUR_STR = "Erreur";
static const char *WARNING_STR = "Attention";
static const char *INFO_STR = "Information";


wchar_t *toWchar(const char *text)
{
	if (NULL == text)
		return NULL;

	wchar_t *buffer = new wchar_t[strlen(text)+1];
	if (0 == MultiByteToWideChar(CP_ACP, 0, text, -1, buffer, strlen(text) + 1))
	{
		CheckError("Internal error: bad string translation", GetLastError());
		delete[] buffer;
		buffer = NULL;
	}

	return buffer;
}

void Error(DWORD msg)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[DEFAULT_BUFSIZE];

	LoadString(hInstance, msg, szMessage, DEFAULT_BUFSIZE);
	MessageBox(hWnd, szMessage, ERREUR_STR, MB_OK | MB_ICONERROR);
}

void Warning(DWORD msg)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[DEFAULT_BUFSIZE];

	LoadString(hInstance, msg, szMessage, DEFAULT_BUFSIZE);
	MessageBox(hWnd, szMessage, WARNING_STR, MB_OK | MB_ICONWARNING);
}

void Info(DWORD msg)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[DEFAULT_BUFSIZE];

	LoadString(hInstance, msg, szMessage, DEFAULT_BUFSIZE);
	MessageBox(hWnd, szMessage, INFO_STR, MB_OK | MB_ICONINFORMATION);
}


void CheckError(const char* msg, DWORD err)
{
	if (err != ERROR_SUCCESS)
	{
		LPVOID lpMsgBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					  FORMAT_MESSAGE_FROM_SYSTEM |
					  FORMAT_MESSAGE_IGNORE_INSERTS,
					  NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					  (LPTSTR)&lpMsgBuf, 0, NULL);

		size_t len = strlen("Error inconnue.") + 1;
		if (NULL != msg)
			len = len + strlen(msg) + 1 + 2;	// NULL char + 2 x newline
		if (NULL != lpMsgBuf)
			len = len + strlen((LPTSTR)lpMsgBuf) + 1 + 1;	// NULL char + newline

		char *buffer = new char[len];
		memset(buffer, 0, len);
		if (NULL != msg)
			strcpy_s(buffer, len, msg);
		strcat_s(buffer, len, "\n\n");
		if (NULL != lpMsgBuf)
			strcat_s(buffer, len, (LPTSTR)lpMsgBuf);
		else
			strcat_s(buffer, len, "Error inconnue.");
		
		MessageBox(hWnd, buffer, ERREUR_STR, MB_OK | MB_ICONERROR);

		if (NULL != lpMsgBuf)
			LocalFree(lpMsgBuf);
		delete[] buffer;
		SetLastError(0);
	}
}

BOOL CheckInstall(const GameCtrlData_st &data)
{
	BOOL res = FindUser("GameCtrl");

	if (TRUE == res)
		for (long i = 0; (TRUE == res) && (i < data.NbGames); i++)
			res = res & CheckSecurity(data.Games[i]);

	return res;
}


BOOL Install(BOOL force)
{
	BOOL res = TRUE;

	if (TRUE == force)
		res = UnInstall(force);

	if (TRUE == res)
		res = CreateUser("GameCtrl");

	if (TRUE == res)
		Info(IDS_INSTALL_SUCCEEDED);
	else
		Error(IDS_INSTALL_FAILED);

	return res;
}

BOOL UnInstall(BOOL force)
{
	BOOL res = DeleteUser("GameCtrl");

	if (TRUE == res)
		Info(IDS_UNINSTALL_SUCCEEDED);
	else
		Error(IDS_UNINSTALL_FAILED);

	return res;
}

BOOL ParseCmdLine(LPSTR lpCmdLine, GameCtrlOptions_st &options)
{
	CHAR *pos = lpCmdLine;
	BOOL res = TRUE;

	memset(&options, 0, sizeof(GameCtrlOptions_st));

	while ((NULL != pos) && (TRUE == res))
	{
		if (0 == *pos)
			pos = NULL;
		else if (' ' == *pos)
			pos = pos + 1;
		else if ('-' == *pos)
		{
			if (NULL == *(pos + 1))
			{
				res = FALSE;	// a dash alone at end of line is an error.
				pos = NULL;
				continue;
			}

			char opt = *(pos + 1);
			if ('-' == opt)
			{
				pos = pos + 2;
				if (pos == strstr(pos, "install"))
				{
					options.doInstall = TRUE;
					pos = pos + 7;
				}
				else if (pos == strstr(pos, "uninstall"))
				{
					options.doUnInstall = TRUE;
					pos = pos + 9;
				}
				else if (pos == strstr(pos, "version"))
				{
					options.doVersion = TRUE;
					pos = pos + 7;
				}
				else if (pos == strstr(pos, "usage"))
				{
					options.doUsage = TRUE;
					pos = pos + 5;
				}
				else if (pos == strstr(pos, "force"))
				{
					options.doForce = TRUE;
					pos = pos + 5;
				}
				else if (pos == strstr(pos, "run"))
				{
					options.doRun = TRUE;
					pos = pos + 5;
				}
				else
					res = FALSE;	// Unknown option
			}
			else
			{
				switch (opt)
				{
					case 'i':
						options.doInstall = TRUE;
						pos = pos + 2;
						break;
					case 'u':
						options.doUnInstall = TRUE;
						pos = pos + 2;
						break;
					case 'h':
						options.doUsage = TRUE;
						pos = pos + 2;
						break;
					case 'v':
						options.doVersion = TRUE;
						pos = pos + 2;
						break;
					case 'f':
						options.doForce = TRUE;
						pos = pos + 2;
						break;
					case 'r':
						options.doRun = TRUE;
						pos = pos + 2;
						break;
					default:
						res = FALSE;	// Unknown option
						break;
				}
			}
		}
		else
			res = FALSE;
	}

	return res;
}

BOOL runGame(const char *path)
{
	if ((NULL == path) || (NULL == hWnd))
		return FALSE;

	if ((0 != pi.hProcess) || (0 != pi.hThread))
	{
		DWORD exitCode = 0;
#pragma warning(suppress: 6387)
		BOOL stillActive = GetExitCodeProcess(pi.hProcess, &exitCode);
		if (FALSE == stillActive)
		{
			CheckError("Unable to collect game liveliness", GetLastError());
			return FALSE;
		}
		else if (0 != exitCode)
		{
			Error(IDS_GAMERUNNING);
			return FALSE;
		}
	}

	wchar_t	*gamePath = toWchar(path);
	STARTUPINFOW siw;
	memset(&siw, 0, sizeof(STARTUPINFOW));
	siw.cb = sizeof(STARTUPINFO);
	siw.lpDesktop = L""; // "winsta0\\default"; Why this does not work even if I add ACEs to Desktop & Winsta ?

	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	if (FALSE == CreateProcessWithLogonW(	L"GameCtrl",
											L".",
											L"GameCtrl",
											0,
											gamePath,
											NULL,
											CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
											NULL, 
											NULL,
											&siw, &pi))
	{
		CheckError("Failed to launch game", ::GetLastError());
		return FALSE;
	}
	else
		// now the job starts
		ResumeThread(pi.hThread);
	
	delete[] gamePath;
	return TRUE;
}

BOOL stopGame(void)
{
	if (NULL == hWnd)
		return FALSE;

	// process already terminated or not started.
	if (0 == pi.hProcess)
		return TRUE;

	BOOL res = TRUE;
	DWORD exitCode = 0;
	BOOL stillActive = GetExitCodeProcess(pi.hProcess,&exitCode);
	if (FALSE == stillActive)
		CheckError("Unable to collect game liveliness",GetLastError());
	else if (0 != exitCode)
	{
		BOOL res = TerminateProcess(pi.hProcess, TRUE);
		if (FALSE == res)
			CheckError("Failed to terminate game", ::GetLastError());
		else
		{
			// Wait until game process exits.
			WaitForSingleObject(pi.hProcess, INFINITE);

			CloseHandle(pi.hProcess);
			if (0 != pi.hThread)
				CloseHandle(pi.hThread);
		}
	}

	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

	return res;
}


BOOL adjustGameTime(GameCtrlData_st &data)
{
	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);
	FILETIME currentTime = { 0, 0 };
	if (FALSE == SystemTimeToFileTime(&SystemTime, &currentTime))
	{
		CheckError("Impossible d'obtenir la date courante:", ::GetLastError());
		return FALSE;
	}

	if ((data.NextUpdateTime.dwHighDateTime == 0) &&
		(data.NextUpdateTime.dwLowDateTime == 0))
	{
		SystemTime.wDay = 1;
		SystemTime.wMonth = 1;
		SystemTime.wYear = 2018;
		SystemTime.wHour = 0;
		SystemTime.wMinute = 0;
		SystemTime.wSecond = 0;
		SystemTime.wMilliseconds = 0;
		SystemTime.wDayOfWeek = 1;
		if (FALSE == SystemTimeToFileTime(&SystemTime, &data.NextUpdateTime))
		{
			CheckError("Impossible d'obtenir la date courante:", ::GetLastError());
			return FALSE;
		}
	}

	LONG cmp = CompareFileTime(&currentTime, &data.NextUpdateTime);
	while (1 == cmp)		// Reset delay is necessary
	{
		data.CHRONO = data.ReinitChrono;
		ULARGE_INTEGER next;
		next.HighPart = data.NextUpdateTime.dwHighDateTime;
		next.LowPart = data.NextUpdateTime.dwLowDateTime;

		ULARGE_INTEGER delta;
		delta.QuadPart = data.NbDaysToReinit * 24 * 60 * 60;
		delta.QuadPart *= 1000 * 1000 * 10;	// in 100ns intervals.

		// TODO: use modulo to satrt at 00:00,
		// i.e. : 864000000000 ns

		next.QuadPart = next.QuadPart + delta.QuadPart;
		data.NextUpdateTime.dwHighDateTime = next.HighPart;
		data.NextUpdateTime.dwLowDateTime = next.LowPart;

		cmp = CompareFileTime(&currentTime, &data.NextUpdateTime);
	}

	return TRUE;
}


BOOL adjustMenu(GameCtrlData_st &data)
{
	if (NULL == hWnd)
		return FALSE;
	HMENU menu = GetMenu(hWnd);
	if (NULL == menu)
		return FALSE;

	HMENU file = GetSubMenu(menu, 0);
	if (NULL == file)
		return FALSE;

	MENUITEMINFO mi;
	memset(&mi, 0, sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_TYPE;
	mi.fType = MFT_SEPARATOR;

	if (FALSE == InsertMenuItem(file, 0, TRUE, &mi))
	{
		CheckError("Impossible d'ajouter une entrée au menu de l'application:", ::GetLastError());
		return FALSE;
	}

	BOOL res = TRUE;
	for (int i = 0; i < data.NbGames; i++)
	{
		HICON hh = ExtractIcon(0, data.Games[i], 0);
		ICONINFO ii;
		GetIconInfo(hh, &ii);

		memset(&mi, 0, sizeof(MENUITEMINFO));
		mi.cbSize = sizeof(MENUITEMINFO);
		mi.fMask = MIIM_STRING | MIIM_ID | MIIM_CHECKMARKS;
		mi.fType = MFT_STRING;
		const char *exe = strrchr(data.Games[i], '\\');
		mi.dwTypeData = (LPSTR)(exe + 1);
		mi.wID = IDM_GAME1 + i;

		BITMAPINFO bi;
		HDC hdc = GetDC(hWnd);
		memset(&bi, 0, sizeof(BITMAPINFO));
		bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		GetDIBits(hdc, ii.hbmColor, 0, 0, NULL, &bi, DIB_RGB_COLORS);
		int size = bi.bmiHeader.biBitCount / 8;
		int dim = bi.bmiHeader.biWidth * bi.bmiHeader.biHeight;
		bi.bmiHeader.biCompression = BI_RGB;	// Extract RGB.

		unsigned char *bits = new unsigned char[dim*size];
		memset(bits, 0, dim*size);
		int nbread = GetDIBits(hdc, ii.hbmColor, 0, bi.bmiHeader.biHeight, bits, &bi, DIB_RGB_COLORS);

		unsigned char *newbits = NULL;
		HBITMAP newBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, (void**)&newbits, NULL, 0);
		if ((NULL == newBitmap) || (newbits == NULL))
		{
			CheckError("Impossible d'obtenir le bitmap de l'icône du jeu:", ::GetLastError());
			return FALSE;
		}
		else
		{
			for (int j = 0; j < bi.bmiHeader.biHeight; j++)
				for (int i = 0; i < bi.bmiHeader.biWidth; i++)
				{
					int pos = 4 * (bi.bmiHeader.biWidth - i - 1 + bi.bmiHeader.biWidth*j);
					int pos2 = 4 * (i + j*bi.bmiHeader.biWidth);

					newbits[pos + 3] = bits[pos2 + 3];
					newbits[pos + 2] = bits[pos2 + 2];
					newbits[pos + 1] = bits[pos2 + 1];
					newbits[pos] = bits[pos2];
				}
		}

		DestroyIcon(hh);
		if (ii.hbmColor != NULL)
			DeleteObject(ii.hbmColor);
		if (ii.hbmMask != NULL)
			DeleteObject(ii.hbmMask);
		delete[] bits;
		mi.hbmpChecked = newBitmap; // ii.hbmColor;
		mi.hbmpUnchecked = newBitmap; // ii.hbmColor;

		if (FALSE == InsertMenuItem(file, 0, TRUE, &mi))
		{
			CheckError("Impossible d'ajouter une entrée de menu.", ::GetLastError());
			res = FALSE;
		}
	}

	return res;
}
