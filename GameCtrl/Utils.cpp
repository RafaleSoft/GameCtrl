// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>


//	Process startup info
STARTUPINFO			si;



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



BOOL runGame(HWND hWnd, const char *path, PROCESS_INFORMATION &pi)
{
	if (NULL == path)
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

