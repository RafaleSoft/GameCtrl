// Utils.cpp : Defines the utility functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>
#include <userenv.h>
#include <tlhelp32.h>
#include <set>

//	Process startup info
static STARTUPINFOW siw;
static PROCESS_INFORMATION	pi;
static const char *ERREUR_STR = "Erreur";
static const char *WARNING_STR = "Attention";
static const char *INFO_STR = "Information";
HWND gameWnd = NULL;
HWND gamechild = NULL;


extern "C"
{
	const char *USER_NAME = "GameCtrl";
	const wchar_t *W_USER_NAME = L"GameCtrl";
	const char *PASSWORD = "RaFaLe99";
	const wchar_t *W_PASSWORD = L"RaFaLe99";
}

static const char *PATCH = "A,NCK+\"2=4JMAA";

std::set<DWORD> process_list;

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
	HINSTANCE	hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[DEFAULT_BUFSIZE];

	LoadString(hInstance, msg, szMessage, DEFAULT_BUFSIZE);
	MessageBox(hWnd, szMessage, ERREUR_STR, MB_OK | MB_ICONERROR);
}

void Warning(DWORD msg)
{
	HINSTANCE	hInstance = GetModuleHandle(NULL);
	TCHAR		szMessage[DEFAULT_BUFSIZE];

	LoadString(hInstance, msg, szMessage, DEFAULT_BUFSIZE);
	MessageBox(hWnd, szMessage, WARNING_STR, MB_OK | MB_ICONWARNING);
}

void Info(DWORD msg)
{
	HINSTANCE	hInstance = GetModuleHandle(NULL);
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

BOOL Reset(void)
{
	char buffer[DEFAULT_BUFSIZE];
	GetModuleFileName(NULL, buffer, DEFAULT_BUFSIZE);
	
	FILE* bin = NULL;
	errno_t err = fopen_s(&bin, buffer, "r+b");
	sprintf_s(buffer, "Reset result: %d", err);
	MessageBox(NULL, buffer, "Reset", MB_OK);

	int nb = fwrite("MZ", 2, 1, bin);
	fclose(bin);

	sprintf_s(buffer, "Reset result2: %d", nb);
	MessageBox(NULL, buffer, "Reset", MB_OK);

	return TRUE;
}

BOOL Install(BOOL force, GameCtrlData_st &data)
{
	BOOL res = TRUE;

	if (TRUE == force)
		res = UnInstall(force);
	
	if (TRUE == res)
		res = CreateUser(USER_NAME, PASSWORD);

	if (TRUE == res)
		res = InitRegistry(data);

	if (TRUE == res)
		Info(IDS_INSTALL_SUCCEEDED);
	else
		Error(IDS_INSTALL_FAILED);

	return res;
}

BOOL UnInstall(BOOL force)
{
	BOOL res = DeleteUser(USER_NAME);

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
				else if (pos == strstr(pos, "reset"))
				{
					options.doReset = TRUE;
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
						options.doReset = TRUE;
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
			CheckError("Impossible de récupérer le statut d'un jeu en cours", GetLastError());
			return FALSE;
		}
		else if (0 != exitCode)
		{
			Error(IDS_GAMERUNNING);
			return FALSE;
		}
	}

	wchar_t	*gamePath = toWchar(path);
	memset(&siw, 0, sizeof(STARTUPINFOW));
	siw.cb = sizeof(STARTUPINFO);
	siw.lpDesktop = L""; // "winsta0\\default"; Why this does not work even if I add ACEs to Desktop & Winsta ?

	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	if (FALSE == CreateProcessWithLogonW(	W_USER_NAME,
											L".",
											W_PASSWORD,
											0,
											gamePath,
											NULL,
											CREATE_SUSPENDED | CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE,
											NULL, 
											NULL,
											&siw, &pi))
	{
		CheckError("Impossible de lancer le jeu", ::GetLastError());
		gameWnd = NULL;	// just a precaution, in case the game exists by itself.
		delete[] gamePath;
		return FALSE;
	}
	else
		// now the job starts
		ResumeThread(pi.hThread);
	
	gameWnd = NULL;	// just a precaution, in case the game exists by itself.
	delete[] gamePath;
	return TRUE;
}


BOOL GetProcessList(DWORD ppid, DWORD *count, DWORD *list)
{
	if (NULL != count)
	{
		// if list is null, the call is only requested to count child processes.
		if (NULL == list)
			*count = 0;
	}
	else
		return FALSE;	// Invalid Parameter.

	// Take a snapshot of all processes in the system.
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		MessageBox(hWnd, "Impossible d'obtenir un snapshot système", ERREUR_STR, MB_OK | MB_ICONINFORMATION);
		return(FALSE);
	}

	// Set the size of the structure before using it.
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	// Retrieve information about the first process,
	// and exit if unsuccessful
	if (!Process32First(hProcessSnap, &pe32))
	{
		CheckError("Impossible d'obtenir le premier processus du snapshot système", GetLastError());
		CloseHandle(hProcessSnap);          // clean the snapshot object
		return(FALSE);
	}

	DWORD counted = 0;
	// Now walk the snapshot of processes, and
	// display information about each process in turn
	do
	{
		if (ppid == pe32.th32ParentProcessID)
		{
			if ((NULL != list) && (counted < *count))
				list[counted] = pe32.th32ProcessID;
			counted++;
		}
	}
	while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
	
	if (NULL == list)
		*count = counted;
	
	return(TRUE);
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD pid = (DWORD)lParam;
	DWORD dwProcessId = 0;

	DWORD threadId = GetWindowThreadProcessId(hwnd,&dwProcessId);
	if (pid == dwProcessId)
	{
		LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
		LONG instance = GetWindowLong(hwnd, GWL_HINSTANCE);
		WINDOWINFO pwi;
		BOOL res = GetWindowInfo(hwnd,&pwi);
		char caption[255];
		int sz = GetWindowTextA(hwnd, caption,255);
		
		LONG style = GetWindowLong(hwnd, GWL_STYLE);
		LONG parent = GetWindowLong(hwnd, GWL_HWNDPARENT);
		if ((NULL == parent) && ((WS_VISIBLE & style) == WS_VISIBLE))
		{
			gameWnd = hwnd;
		}
	}
		
	return TRUE;
}

BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam)
{
	if (NULL != gamechild)
		return TRUE;

	LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	LONG style = GetWindowLong(hwnd, GWL_STYLE);
	//LONG parent = GetWindowLong(hwnd, GWL_HWNDPARENT);
	//LONG instance = GetWindowLong(hwnd, GWL_HINSTANCE);

	WINDOWINFO pwi_parent;
	BOOL res = GetWindowInfo(gameWnd, &pwi_parent);
	WINDOWINFO pwi;
	res = res && GetWindowInfo(hwnd, &pwi);

	char caption[255];
	int sz = GetWindowTextA(hwnd, caption, 255);

	if ((pwi.rcClient.left >= pwi_parent.rcClient.left) &&
		(pwi.rcClient.right <= pwi_parent.rcClient.right) &&
		(pwi.rcClient.bottom <= pwi_parent.rcClient.bottom) &&
		(pwi.rcClient.top >= pwi_parent.rcClient.top))
	{
		//if (!strcmp(caption, "_ctl.Window"))
			gamechild = hwnd;
	}

	return TRUE;
}

HWND GetWindowGame(void)
{
	if ((NULL != gameWnd) && (NULL != gamechild))
		return gameWnd;

	BOOL res = TRUE;
	// First try to find the main game window
	if (NULL == gameWnd)
		res = EnumWindows(EnumWindowsProc, pi.dwProcessId);

	if ((FALSE == res) || (NULL == gameWnd))
		return NULL;
	else
	{
		// then try to find any child window actually hosting the game display
		if (NULL == gamechild)
			res = EnumChildWindows(gameWnd, EnumChildWindowsProc, NULL);

		return gameWnd;
	}
}

BOOL UpdateProcessTree(DWORD rootPid)
{
	// then try to find any child window actually hosting the game display
	BOOL e = FALSE;
	if ((NULL != gameWnd) && (NULL == gamechild))
		e = EnumChildWindows(gameWnd, EnumChildWindowsProc, NULL);
	
	if ((0 == rootPid) && (0 == pi.dwProcessId))
		return FALSE;

	DWORD pid = rootPid;
	if (0 == pid)
		pid = pi.dwProcessId;

	//	Get child processes
	DWORD count = 0;
	DWORD *tmplist = NULL;
	BOOL res = GetProcessList(pid, &count, NULL);
	if (res && (count > 0))
	{
		tmplist = new DWORD[count];
		res = GetProcessList(pid, &count, tmplist);
	}

	for (size_t i = 0; i < count; i++)
		process_list.insert(tmplist[i]);
	delete[] tmplist;

	return res;
}

BOOL TerminateProcessTree(DWORD rootPid)
{
	BOOL res = UpdateProcessTree(rootPid);

	while (process_list.size() > 0)
	{
		//count--;
		DWORD pid = *(process_list.rbegin());

		// Get HANDLE from pid.
		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);

		// Do not check result because it will surely fail if parent process closed a child.
		// To improve, add a check to verify pid existence.
		TerminateProcess(hProcess, 1);

		// Wait until game process exits.
		WaitForSingleObject(hProcess, INFINITE);
		CloseHandle(hProcess);

		//	Terminate process grand-childs if any (might have terminate with its parent here above)
		process_list.erase(pid);
		res = res && TerminateProcessTree(pid);
	}

	return res;
}

BOOL checkLiveliness(void)
{
	// process already terminated or not started.
	if (0 == pi.hProcess)
		return TRUE;

	BOOL terminate = FALSE;
	DWORD exitCode = 0;
	BOOL stillActive = GetExitCodeProcess(pi.hProcess, &exitCode);
	if (FALSE == stillActive)
		terminate = FALSE;
	else if (STILL_ACTIVE == exitCode)
		terminate = TRUE;

	return terminate;
}

BOOL stopGame(void)
{
	if (NULL == hWnd)
		return FALSE;

	// process already terminated or not started.
	if (0 == pi.hProcess)
		return TRUE;

	//	Collect all processes child of running game.
	BOOL res = TRUE;
	DWORD exitCode = 0;
	BOOL stillActive = GetExitCodeProcess(pi.hProcess,&exitCode);
	if (FALSE == stillActive)
		CheckError("Unable to collect game liveliness",GetLastError());
	else if (STILL_ACTIVE == exitCode)
	{
		// First try to grafully ask the application to close
		if (NULL != GetWindowGame())
		{
			res = (0 != SendMessage(gameWnd, WM_CLOSE, 0, 0));
			Sleep(500);
			if (FALSE == res)	// Then send a system close
			{
				res = (0 != SendMessage(gameWnd, SC_CLOSE, 0, 0));
				Sleep(500);
			}
		}

		// If not friendly, be more firm !
		stillActive = GetExitCodeProcess(pi.hProcess, &exitCode);
		if (STILL_ACTIVE == exitCode)
		{
			res = TerminateProcess(pi.hProcess, 1);
			if (FALSE == res)
				CheckError("Failed to terminate game", ::GetLastError());
			else
			{
				// Wait until game process exits.
				// TODO : prevent infinite wait here with a multiple attempt loop.
				WaitForSingleObject(pi.hProcess, INFINITE);
				CloseHandle(pi.hProcess);
				if (0 != pi.hThread)
					CloseHandle(pi.hThread);
			}
		}
	}

	res = res && TerminateProcessTree(pi.dwProcessId);
	process_list.clear();

	memset(&siw, 0, sizeof(STARTUPINFOW));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));
	gameWnd = NULL;

	return res;
}

BOOL checkTimeSlot(GameCtrlData_st &data, BOOL& lastminute)
{
	BOOL res = FALSE;

	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);

	// make Monday = day 0
	unsigned int day = (SystemTime.wDayOfWeek + 6) % 7;
	unsigned int hour = SystemTime.wHour;
	lastminute = (SystemTime.wMinute == 59 ? TRUE: FALSE);

	unsigned char h = data.HourSlots[hour];
	res = (((h >> day) & 0x1) ? TRUE : FALSE);

	return res;
}

BOOL adjustGameTime(GameCtrlData_st &data)
{
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	FILETIME currentTime = { 0, 0 };
	if (FALSE == SystemTimeToFileTime(&SystemTime, &currentTime))
	{
		CheckError("Impossible d'obtenir la date courante:", GetLastError());
		return FALSE;
	}

	if ((data.NextUpdateTime.dwHighDateTime == 0) &&
		(data.NextUpdateTime.dwLowDateTime == 0))
	{
		SystemTime.wDay = 1;
		SystemTime.wMonth = 1;
		SystemTime.wYear = 2019;
		SystemTime.wHour = 0;
		SystemTime.wMinute = 0;
		SystemTime.wSecond = 0;
		SystemTime.wMilliseconds = 0;
		SystemTime.wDayOfWeek = 1;
		if (FALSE == SystemTimeToFileTime(&SystemTime, &data.NextUpdateTime))
		{
			CheckError("Impossible d'obtenir la date courante:", GetLastError());
			return FALSE;
		}
	}

	// Next update time.
	//if (FALSE == FileTimeToSystemTime(&data.NextUpdateTime, &SystemTime))
	//{
	//	CheckError("Impossible d'obtenir la date de reset:", GetLastError());
	//}

	// TODO: use modulo to start at 00:00,
	// i.e. : 864000000000 ns

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

		next.QuadPart = next.QuadPart + delta.QuadPart;
		data.NextUpdateTime.dwHighDateTime = next.HighPart;
		data.NextUpdateTime.dwLowDateTime = next.LowPart;

		cmp = CompareFileTime(&currentTime, &data.NextUpdateTime);
	}

	return TRUE;
}


BOOL CheckInstall(GameCtrlData_st &data)
{
	if (!GetRegistryVars(data))
		return FALSE;
	else if (FALSE == adjustGameTime(data))
		return FALSE;
	else if (FALSE == adjustMenu(data))
		return FALSE;

	BOOL res = FindUser(USER_NAME);

	if (TRUE == res)
		for (long i = 0; (TRUE == res) && (i < data.NbGames); i++)
			res = res & CheckSecurity(data.Games[i]);

#ifdef SHAREWARE
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	FILETIME currentTime = { 0, 0 };
	if (FALSE == SystemTimeToFileTime(&SystemTime, &currentTime))
	{
		CheckError("Impossible d'obtenir la date courante:", GetLastError());
		return FALSE;
	}

	char buffer[DEFAULT_BUFSIZE];
	memset(buffer, 0, DEFAULT_BUFSIZE);
	const char *info = "Logiciel utilisable jusqu'au: \n\n";
	size_t infolen = strlen(info);
	strcpy_s(buffer, DEFAULT_BUFSIZE, info);

	SYSTEMTIME LastSystemTime;
	memset(&LastSystemTime, 0, sizeof(SYSTEMTIME));

	LastSystemTime.wDay = *((unsigned char*)&PATCH[10]);
	LastSystemTime.wMonth = *((unsigned char*)&PATCH[11]);
	LastSystemTime.wYear = *((unsigned short*)&PATCH[12]);
	
	sprintf_s(buffer + infolen, DEFAULT_BUFSIZE - infolen, "%d / %d / %d", LastSystemTime.wDay, LastSystemTime.wMonth, LastSystemTime.wYear);
	
	FILETIME LastUpdateTime;
	SystemTimeToFileTime(&LastSystemTime, &LastUpdateTime);

	LONG cmp = CompareFileTime(&currentTime, &LastUpdateTime);
	if (1 == cmp)		// Reset delay is necessary
	{
		Error(IDS_LIMITREACHED);
		return FALSE;
	}
	else
	{
		MessageBox(hWnd, buffer, INFO_STR, MB_OK | MB_ICONINFORMATION);
	}
#endif

	return res;
}


/*


BOOL GetModuleList(DWORD ppid, DWORD *count, DWORD *list)
{
if (NULL != count)
{
// if list is null, the call is only requested to count child processes.
if (NULL == list)
*count = 0;
}
else
return FALSE;	// Invalid Parameter.

// Take a snapshot of all processes in the system.
HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ppid);
if (hProcessSnap == INVALID_HANDLE_VALUE)
{
DWORD err = GetLastError();
MessageBox(hWnd, "Impossible d'obtenir un snapshot système", ERREUR_STR, MB_OK | MB_ICONINFORMATION);
return(FALSE);
}

// Set the size of the structure before using it.
MODULEENTRY32 me32;
me32.dwSize = sizeof(MODULEENTRY32);

// Retrieve information about the first process,
// and exit if unsuccessful
if (!Module32First(hProcessSnap, &me32))
{
CheckError("Impossible d'obtenir le premier processus du snapshot système", GetLastError());
CloseHandle(hProcessSnap);          // clean the snapshot object
return(FALSE);
}

DWORD counted = 0;
// Now walk the snapshot of processes, and
// display information about each process in turn
do
{
if (ppid == me32.th32ProcessID)
{
if ((NULL != list) && (counted < *count))
list[counted] = me32.th32ProcessID;
counted++;
}
} while (Module32Next(hProcessSnap, &me32));

CloseHandle(hProcessSnap);

if (NULL == list)
*count = counted;

return(TRUE);
}


*/
/*
//  Forward declarations:
BOOL ListProcessModules( DWORD dwPID );
BOOL ListProcessThreads( DWORD dwOwnerPID );
void printError( TCHAR* msg );

BOOL ListProcessModules( DWORD dwPID )
{
HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
MODULEENTRY32 me32;

// Take a snapshot of all modules in the specified process.
hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
if( hModuleSnap == INVALID_HANDLE_VALUE )
{
printError( TEXT("CreateToolhelp32Snapshot (of modules)") );
return( FALSE );
}

// Set the size of the structure before using it.
me32.dwSize = sizeof( MODULEENTRY32 );

// Retrieve information about the first module,
// and exit if unsuccessful
if( !Module32First( hModuleSnap, &me32 ) )
{
printError( TEXT("Module32First") );  // show cause of failure
CloseHandle( hModuleSnap );           // clean the snapshot object
return( FALSE );
}

// Now walk the module list of the process,
// and display information about each module
do
{
_tprintf( TEXT("\n\n     MODULE NAME:     %s"),   me32.szModule );
_tprintf( TEXT("\n     Executable     = %s"),     me32.szExePath );
_tprintf( TEXT("\n     Process ID     = 0x%08X"),         me32.th32ProcessID );
_tprintf( TEXT("\n     Ref count (g)  = 0x%04X"),     me32.GlblcntUsage );
_tprintf( TEXT("\n     Ref count (p)  = 0x%04X"),     me32.ProccntUsage );
_tprintf( TEXT("\n     Base address   = 0x%08X"), (DWORD) me32.modBaseAddr );
_tprintf( TEXT("\n     Base size      = %d"),             me32.modBaseSize );

} while( Module32Next( hModuleSnap, &me32 ) );

CloseHandle( hModuleSnap );
return( TRUE );
}

BOOL ListProcessThreads( DWORD dwOwnerPID )
{
HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
THREADENTRY32 te32;

// Take a snapshot of all running threads
hThreadSnap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
if( hThreadSnap == INVALID_HANDLE_VALUE )
return( FALSE );

// Fill in the size of the structure before using it.
te32.dwSize = sizeof(THREADENTRY32);

// Retrieve information about the first thread,
// and exit if unsuccessful
if( !Thread32First( hThreadSnap, &te32 ) )
{
printError( TEXT("Thread32First") ); // show cause of failure
CloseHandle( hThreadSnap );          // clean the snapshot object
return( FALSE );
}

// Now walk the thread list of the system,
// and display information about each thread
// associated with the specified process
do
{
if( te32.th32OwnerProcessID == dwOwnerPID )
{
_tprintf( TEXT("\n\n     THREAD ID      = 0x%08X"), te32.th32ThreadID );
_tprintf( TEXT("\n     Base priority  = %d"), te32.tpBasePri );
_tprintf( TEXT("\n     Delta priority = %d"), te32.tpDeltaPri );
_tprintf( TEXT("\n"));
}
} while( Thread32Next(hThreadSnap, &te32 ) );

CloseHandle( hThreadSnap );
return( TRUE );
}
*/