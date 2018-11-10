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

		const char *msg = (NULL != lpMsgBuf) ? (const char*)lpMsgBuf : "Unknown Win32 error.";
		MessageBox(hWnd, (LPTSTR)lpMsgBuf, ERREUR_STR, MB_OK | MB_ICONERROR);

		if (NULL != lpMsgBuf)
			LocalFree(lpMsgBuf);
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

	wchar_t	gamePath[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, path, -1, gamePath, MAX_PATH);

	STARTUPINFOW siw;
	memset(&siw, 0, sizeof(STARTUPINFOW));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	siw.cb = sizeof(STARTUPINFO);
	siw.lpDesktop = L""; // "winsta0\\default";

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
	
	return TRUE;
}

BOOL stopGame(void)
{
	if (NULL == hWnd)
		return FALSE;

	// process already terminated or not started.
	if (0 == pi.hProcess)
		return TRUE;

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

		memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	}

	return res;
}


void adjustGameTime(GameCtrlData_st &data)
{
	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);
	FILETIME currentTime;
	SystemTimeToFileTime(&SystemTime, &currentTime);

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
		SystemTimeToFileTime(&SystemTime, &data.NextUpdateTime);
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
		return FALSE;

	HICON hh = ExtractIcon(0, data.Games[0], 0);
	ICONINFO ii;
	GetIconInfo(hh, &ii);

	memset(&mi, 0, sizeof(MENUITEMINFO));
	mi.cbSize = sizeof(MENUITEMINFO);
	mi.fMask = MIIM_STRING | MIIM_ID | MIIM_CHECKMARKS;
	mi.fType = MFT_STRING;
	const char *exe = strrchr(data.Games[0], '\\');
	mi.dwTypeData = (LPSTR)(exe + 1);
	mi.wID = 1024;
	mi.hbmpChecked = ii.hbmColor;
	mi.hbmpUnchecked = ii.hbmColor;

	if (FALSE == InsertMenuItem(file, 0, TRUE, &mi))
		return FALSE;

	return TRUE;
}
