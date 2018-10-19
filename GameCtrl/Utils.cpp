// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>


//	Process startup info
static STARTUPINFO si;
static PROCESS_INFORMATION	pi;
static const char *ERREUR_STR = "Erreur";
static const char *WARNING_STR = "Attention";

#define MAX_LOADSTRING 256

void Error(DWORD msg)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[MAX_LOADSTRING];

	LoadString(hInstance, msg, szMessage, MAX_LOADSTRING);
	MessageBox(hWnd, szMessage, ERREUR_STR, MB_OK | MB_ICONERROR);
}

void Warning(DWORD msg)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[MAX_LOADSTRING];

	LoadString(hInstance, msg, szMessage, MAX_LOADSTRING);
	MessageBox(hWnd, szMessage, WARNING_STR, MB_OK | MB_ICONWARNING);
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

BOOL CheckInstall(GameCtrlData_st &data)
{
	BOOL res = TRUE;

	if (FALSE == FindUser("GameCtrl"))
		res = FALSE;

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

	char				gamePath[MAX_PATH];
	char				gameArgs[MAX_PATH];

	sprintf_s(gamePath, "%s", path);
	sprintf_s(gameArgs, "%s", "");

	memset(&si, 0, sizeof(STARTUPINFO));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
	si.lpTitle = "Play Game";

	DWORD	creationFlag = CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE;

	// creating work unit

	if (0 == CreateProcess(	gamePath,		// pointer to name of executable module
							gameArgs,		// pointer to command line string
							NULL,			// process security attributes
							NULL,			// thread security attributes
							TRUE,			// handle inheritance flag
							creationFlag,	// creation flags
							NULL,			// pointer to new environment block
							NULL,			// pointer to current directory name
							&si,			// pointer to STARTUPINFO
							&pi))			// pointer to PROCESS_INFORMATION
	{
		CheckError("Error launching game", ::GetLastError());
		return FALSE;
	}


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


BOOL Install(void)
{
	BOOL res = false;

	if (TRUE == FindUser("GameCtrl"))
		DeleteUser("GameCtrl");
	
	Warning(IDS_CREATEUSER);
	CreateUser("GameCtrl");

	return res;
}

BOOL UnInstall(void)
{
	BOOL res = false;

	DeleteUser("GameCtrl");

	return res;
}


