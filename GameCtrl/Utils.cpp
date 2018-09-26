// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>


//	Process startup info
static STARTUPINFO si;
static PROCESS_INFORMATION	pi;


void CheckError(HWND hWnd, const char* msg, DWORD err)
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
		::MessageBox(hWnd, (LPTSTR)lpMsgBuf, msg, MB_OK);

		if (NULL != lpMsgBuf)
			LocalFree(lpMsgBuf);
		SetLastError(0);
	}
}



BOOL runGame(HWND hWnd, const char *path)
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

	if (0 == CreateProcess(gamePath,		// pointer to name of executable module
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
		CheckError(hWnd, "Error launching game", ::GetLastError());
		return FALSE;
	}


	// now the job starts
	::ResumeThread(pi.hThread);

	return TRUE;
}

BOOL stopGame(HWND hWnd)
{
	if (NULL == hWnd)
		return FALSE;

	// process already terminated or not started.
	if (0 == pi.hProcess)
		return TRUE;

	BOOL res = TerminateProcess(pi.hProcess, TRUE);
	if (FALSE == res)
		CheckError(hWnd, "Failed to terminate game", ::GetLastError());
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
	if (1 == cmp)		// Reset delay is necessary
	{
		data.CHRONO = data.ReinitChrono;
		ULARGE_INTEGER next;
		next.HighPart = data.NextUpdateTime.dwHighDateTime;
		next.LowPart = data.NextUpdateTime.dwLowDateTime;

		ULARGE_INTEGER delta;
		delta.QuadPart = data.NbDaysToReinit * 24 * 60 * 60;
		delta.QuadPart *= 1000 * 1000 * 10;	// in 100ns intervals.

		next.QuadPart = next.QuadPart + delta.QuadPart;
		data.NextUpdateTime.dwHighDateTime = next.HighPart;
		data.NextUpdateTime.dwLowDateTime = next.LowPart;
	}
}
