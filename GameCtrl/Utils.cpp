// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>
#include <lm.h>

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
		MessageBox(hWnd, (LPTSTR)lpMsgBuf, "Erreur", MB_OK);

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
	ResumeThread(pi.hThread);

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
}



BOOL IsUserAdmin(HANDLE token)
{
	BOOL b = FALSE;

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;

	b = AllocateAndInitializeSid(&NtAuthority,
								 2,
								 SECURITY_BUILTIN_DOMAIN_RID,
								 DOMAIN_ALIAS_RID_ADMINS,
								 0, 0, 0, 0, 0, 0,
								 &AdministratorsGroup);
	if (TRUE == b)
	{
		if (!CheckTokenMembership(token, AdministratorsGroup, &b))
		{
			b = FALSE;
		}
		FreeSid(AdministratorsGroup);
	}

	return(b);
}

LPVOID RetrieveTokenInformationClass(HANDLE hToken, TOKEN_INFORMATION_CLASS InfoClass, LPDWORD lpdwSize)
{
	LPVOID pInfo = NULL;
	BOOL fSuccess = FALSE;

	*lpdwSize = 0;

	//
	// Determine the size of the buffer needed
	//

	GetTokenInformation(hToken, InfoClass, NULL, *lpdwSize, lpdwSize);
	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		_tprintf(_T("GetTokenInformation failed with %d\n"), GetLastError());
		return NULL;
	}

	//
	// Allocate a buffer for getting token information
	//
	pInfo = LocalAlloc(LPTR, *lpdwSize);
	if (pInfo == NULL)
	{
		_tprintf(_T("LocalAlloc failed with %d\n"), GetLastError());
		return NULL;
	}

	if (!GetTokenInformation(hToken, InfoClass, pInfo, *lpdwSize, lpdwSize))
	{
		_tprintf(_T("GetTokenInformation failed with %d\n"), GetLastError());
		return NULL;
	}

	fSuccess = TRUE;

	
	// Free pDomainAndUserName only if failed
	// Otherwise, the caller has to free after use
	if (fSuccess == FALSE)
		if (NULL != pInfo)
			LocalFree(pInfo);

	return pInfo;
}


BOOL IsGuest(HANDLE token)
{
	BOOL b = FALSE;

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID GuestsGroup;

	b = AllocateAndInitializeSid(&NtAuthority,
								 1,
								 DOMAIN_GROUP_RID_GUESTS, //DOMAIN_USER_RID_GUEST,
								 0,
								 0, 0, 0, 0, 0, 0,
								 &GuestsGroup);
	if (TRUE == b)
	{
		if (!CheckTokenMembership(token, GuestsGroup, &b))
		{
			b = FALSE;
		}
		FreeSid(GuestsGroup);
	}

	return(b);
}

#include <wincred.h>
BOOL CreateUser(const char* UserName)
{
	USER_INFO_1  ui;
	ZeroMemory(&ui, sizeof(ui));

	wchar_t uname[256];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, 256);
	wchar_t pwd[256];
	MultiByteToWideChar(CP_ACP, 0, "RaFaLe99", -1, pwd, 256);
	ui.usri1_name = uname;
	ui.usri1_password = pwd;
	ui.usri1_priv = USER_PRIV_USER;
	ui.usri1_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT;


	DWORD dwLevel = 2;
	DWORD dwError = -1;
	NET_API_STATUS status = NetUserAdd(NULL, dwLevel, (LPBYTE)&ui, &dwError);

	BOOL res = FALSE;
	switch (status)
	{
		case ERROR_ACCESS_DENIED:
			MessageBox(NULL, "Exécuter le programme avec un rôle administrateur", "Privilèges insuffisants", MB_OK);
			break;
		case NERR_InvalidComputer:
		case NERR_NotPrimary:
		case NERR_GroupExists:
		case NERR_UserExists:
		case NERR_PasswordTooShort:
			// Nothing done here
			break;
		case NERR_Success:
		{
			MessageBox(NULL, "Compte Créé, ajout au groupe des utilisateurs", "Configuration GameCtrl", MB_OK);

			SID users;
			DWORD sidSize = 0;
			CreateWellKnownSid(WinBuiltinUsersSid, NULL,  &users, &sidSize);

			TCHAR GroupName[256];
			TCHAR DomainName[256];
			DWORD bufferSize = 256;
			SID_NAME_USE use = SidTypeGroup;
			LookupAccountSid(NULL, &users, GroupName, &bufferSize, DomainName, &bufferSize, &use);


			wchar_t gname[256];
			MultiByteToWideChar(CP_ACP, 0, GroupName, -1, uname, 256);
			LOCALGROUP_MEMBERS_INFO_0 ginfo;
			ginfo.lgrmi0_sid = 0;
			status = NetLocalGroupAddMembers(	NULL,	// LPCWSTR  ServerName, NULL is local
												gname,
												0,		// DWORD   level,
												(LPBYTE)&ginfo,
												1);
			res = (NERR_Success == status);
			break;
		}
		default:
			MessageBox(NULL, "Erreur inconnue", "Erreur inconnue", MB_OK);
	}

	return res;
}
BOOL DeleteUser(const char* UserName)
{
	wchar_t uname[256];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, 256);

	NET_API_STATUS status = NetUserDel(NULL, uname);

	BOOL res = FALSE;
	switch (status)
	{
		case ERROR_ACCESS_DENIED:
			MessageBox(NULL, "Exécuter le programme avec un rôle administrateur", "Privilèges insuffisants", MB_OK);
			break;
		case NERR_InvalidComputer:
		case NERR_NotPrimary:
			// Nothing done here
			break;
		case NERR_UserNotFound:
			// Nothing done here
			MessageBox(NULL, "Utilisateur inconnu", "Erreur de login", MB_OK);
			break;
		case NERR_Success:
		{
			res = TRUE;
			break;
		}
	}

	return res;
}

BOOL FindUser(const char* UserName)
{
	DWORD ReturnedEntryCount = 0;
	PVOID SortedBuffer = NULL;
	NET_API_STATUS status = NetQueryDisplayInformation(NULL,	// LPCWSTR  ServerName, NULL is local
													   1,		// IN DWORD    Level, User informations
													   0,		// IN DWORD    Index,
													   64,		// IN DWORD    EntriesRequested,
													   MAX_PREFERRED_LENGTH,	// IN DWORD    PreferredMaximumLength,
													   &ReturnedEntryCount,
													   &SortedBuffer);
	BOOL res = FALSE;
	switch (status)
	{
		case ERROR_ACCESS_DENIED:
			MessageBox(NULL, "Exécuter le programme avec un rôle administrateur", "Privilèges insuffisants", MB_OK);
			break;
		case ERROR_INVALID_LEVEL:
		case ERROR_MORE_DATA:
			// Nothing done here
			break;
		case NERR_Success:
		{
			NET_DISPLAY_USER *users = (NET_DISPLAY_USER *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
			{
				char buffer[256];
				int size = 0;
				WideCharToMultiByte(CP_ACP, 0, users[i].usri1_name, -1, buffer, 256, NULL, NULL);
				if (!strcmp(UserName, buffer))
				{
					res = TRUE;
				}
			}
			break;
		}
		default:
			MessageBox(NULL, "Erreur inconnue", "Erreur inconnue", MB_OK);
	}

	if (NULL != SortedBuffer)
		NetApiBufferFree(SortedBuffer);
	return res;
}


BOOL FindGroup(const char* GroupName)
{
	DWORD ReturnedEntryCount = 0;
	PVOID SortedBuffer = NULL;
	NET_API_STATUS status = NetQueryDisplayInformation(NULL,	// LPCWSTR  ServerName, NULL is local
													   3,		// IN DWORD    Level, Group informations
													   0,		// IN DWORD    Index,
													   64,		// IN DWORD    EntriesRequested,
													   MAX_PREFERRED_LENGTH,	// IN DWORD    PreferredMaximumLength,
													   &ReturnedEntryCount,
													   &SortedBuffer);
	BOOL res = FALSE;
	switch (status)
	{
		case ERROR_ACCESS_DENIED:
			MessageBox(NULL, "Exécuter le programme avec un rôle administrateur", "Privilèges insuffisants", MB_OK);
			break;
		case ERROR_INVALID_LEVEL:
		case ERROR_MORE_DATA:
			// Nothing done here
			break;
		case NERR_Success:
		{
			NET_DISPLAY_GROUP *groups = (NET_DISPLAY_GROUP *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
			{
				char buffer[256];
				int size = 0;
				WideCharToMultiByte(CP_ACP, 0, groups[i].grpi3_name, -1, buffer, 256, NULL, NULL);
				if (!strcmp(GroupName, buffer))
				{
					res = TRUE;
				}
			}
			break;
		}
		default:
			MessageBox(NULL, "Erreur inconnue", "Erreur inconnue", MB_OK);
	}

	if (NULL != SortedBuffer)
		NetApiBufferFree(SortedBuffer);
	return res;
}

/*


#include <windows.h>
#include <Tchar.h>

#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | \
GENERIC_EXECUTE | GENERIC_ALL)

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid);

BOOL AddAceToDesktop(HDESK hdesk, PSID psid);

BOOL GetLogonSID (HANDLE hToken, PSID *ppsid);

VOID FreeLogonSID (PSID *ppsid);

BOOL StartInteractiveClientProcess (
LPTSTR lpszUsername,    // client to log on
LPTSTR lpszDomain,      // domain of client's account
LPTSTR lpszPassword,    // client's password
LPTSTR lpCommandLine    // command line to execute
)
{
HANDLE      hToken;
HDESK       hdesk = NULL;
HWINSTA     hwinsta = NULL, hwinstaSave = NULL;
PROCESS_INFORMATION pi;
PSID pSid = NULL;
STARTUPINFO si;
BOOL bResult = FALSE;

// Log the client on to the local computer.

if (!LogonUser(
lpszUsername,
lpszDomain,
lpszPassword,
LOGON32_LOGON_INTERACTIVE,
LOGON32_PROVIDER_DEFAULT,
&hToken) )
{
goto Cleanup;
}

// Save a handle to the caller's current window station.

if ( (hwinstaSave = GetProcessWindowStation() ) == NULL)
goto Cleanup;

// Get a handle to the interactive window station.

hwinsta = OpenWindowStation(
_T("winsta0"),                   // the interactive window station
FALSE,                       // handle is not inheritable
READ_CONTROL | WRITE_DAC);   // rights to read/write the DACL

if (hwinsta == NULL)
goto Cleanup;

// To get the correct default desktop, set the caller's
// window station to the interactive window station.

if (!SetProcessWindowStation(hwinsta))
goto Cleanup;

// Get a handle to the interactive desktop.

hdesk = OpenDesktop(
_T("default"),     // the interactive window station
0,             // no interaction with other desktop processes
FALSE,         // handle is not inheritable
READ_CONTROL | // request the rights to read and write the DACL
WRITE_DAC |
DESKTOP_WRITEOBJECTS |
DESKTOP_READOBJECTS);

// Restore the caller's window station.

if (!SetProcessWindowStation(hwinstaSave))
goto Cleanup;

if (hdesk == NULL)
goto Cleanup;

// Get the SID for the client's logon session.

if (!GetLogonSID(hToken, &pSid))
goto Cleanup;

// Allow logon SID full access to interactive window station.

if (! AddAceToWindowStation(hwinsta, pSid) )
goto Cleanup;

// Allow logon SID full access to interactive desktop.

if (! AddAceToDesktop(hdesk, pSid) )
goto Cleanup;

// Impersonate client to ensure access to executable file.

if (! ImpersonateLoggedOnUser(hToken) )
goto Cleanup;

// Initialize the STARTUPINFO structure.
// Specify that the process runs in the interactive desktop.

ZeroMemory(&si, sizeof(STARTUPINFO));
si.cb= sizeof(STARTUPINFO);
si.lpDesktop = TEXT("winsta0\\default");

// Launch the process in the client's logon session.

bResult = CreateProcessAsUser(
hToken,            // client's access token
NULL,              // file to execute
lpCommandLine,     // command line
NULL,              // pointer to process SECURITY_ATTRIBUTES
NULL,              // pointer to thread SECURITY_ATTRIBUTES
FALSE,             // handles are not inheritable
NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,   // creation flags
NULL,              // pointer to new environment block
NULL,              // name of current directory
&si,               // pointer to STARTUPINFO structure
&pi                // receives information about new process
);

// End impersonation of client.

RevertToSelf();

if (bResult && pi.hProcess != INVALID_HANDLE_VALUE)
{
WaitForSingleObject(pi.hProcess, INFINITE);
CloseHandle(pi.hProcess);
}

if (pi.hThread != INVALID_HANDLE_VALUE)
CloseHandle(pi.hThread);

Cleanup:

if (hwinstaSave != NULL)
SetProcessWindowStation (hwinstaSave);

// Free the buffer for the logon SID.

if (pSid)
FreeLogonSID(&pSid);

// Close the handles to the interactive window station and desktop.

if (hwinsta)
CloseWindowStation(hwinsta);

if (hdesk)
CloseDesktop(hdesk);

// Close the handle to the client's access token.

if (hToken != INVALID_HANDLE_VALUE)
CloseHandle(hToken);

return bResult;
}

BOOL AddAceToWindowStation(HWINSTA hwinsta, PSID psid)
{
ACCESS_ALLOWED_ACE   *pace = NULL;
ACL_SIZE_INFORMATION aclSizeInfo;
BOOL                 bDaclExist;
BOOL                 bDaclPresent;
BOOL                 bSuccess = FALSE;
DWORD                dwNewAclSize;
DWORD                dwSidSize = 0;
DWORD                dwSdSizeNeeded;
PACL                 pacl;
PACL                 pNewAcl = NULL;
PSECURITY_DESCRIPTOR psd = NULL;
PSECURITY_DESCRIPTOR psdNew = NULL;
PVOID                pTempAce;
SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
unsigned int         i;

__try
{
// Obtain the DACL for the window station.

if (!GetUserObjectSecurity(
hwinsta,
&si,
psd,
dwSidSize,
&dwSdSizeNeeded)
)
if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
{
psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwSdSizeNeeded);

if (psd == NULL)
__leave;

psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwSdSizeNeeded);

if (psdNew == NULL)
__leave;

dwSidSize = dwSdSizeNeeded;

if (!GetUserObjectSecurity(
hwinsta,
&si,
psd,
dwSidSize,
&dwSdSizeNeeded)
)
__leave;
}
else
__leave;

// Create a new DACL.

if (!InitializeSecurityDescriptor(
psdNew,
SECURITY_DESCRIPTOR_REVISION)
)
__leave;

// Get the DACL from the security descriptor.

if (!GetSecurityDescriptorDacl(
psd,
&bDaclPresent,
&pacl,
&bDaclExist)
)
__leave;

// Initialize the ACL.

ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
aclSizeInfo.AclBytesInUse = sizeof(ACL);

// Call only if the DACL is not NULL.

if (pacl != NULL)
{
// get the file ACL size info
if (!GetAclInformation(
pacl,
(LPVOID)&aclSizeInfo,
sizeof(ACL_SIZE_INFORMATION),
AclSizeInformation)
)
__leave;
}

// Compute the size of the new ACL.

dwNewAclSize = aclSizeInfo.AclBytesInUse +
(2*sizeof(ACCESS_ALLOWED_ACE)) + (2*GetLengthSid(psid)) -
(2*sizeof(DWORD));

// Allocate memory for the new ACL.

pNewAcl = (PACL)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwNewAclSize);

if (pNewAcl == NULL)
__leave;

// Initialize the new DACL.

if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
__leave;

// If DACL is present, copy it to a new DACL.

if (bDaclPresent)
{
// Copy the ACEs to the new ACL.
if (aclSizeInfo.AceCount)
{
for (i=0; i < aclSizeInfo.AceCount; i++)
{
// Get an ACE.
if (!GetAce(pacl, i, &pTempAce))
__leave;

// Add the ACE to the new ACL.
if (!AddAce(
pNewAcl,
ACL_REVISION,
MAXDWORD,
pTempAce,
((PACE_HEADER)pTempAce)->AceSize)
)
__leave;
}
}
}

// Add the first ACE to the window station.

pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
sizeof(DWORD));

if (pace == NULL)
__leave;

pace->Header.AceType  = ACCESS_ALLOWED_ACE_TYPE;
pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
pace->Header.AceSize  = LOWORD(sizeof(ACCESS_ALLOWED_ACE) +
GetLengthSid(psid) - sizeof(DWORD));
pace->Mask            = GENERIC_ACCESS;

if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
__leave;

if (!AddAce(
pNewAcl,
ACL_REVISION,
MAXDWORD,
(LPVOID)pace,
pace->Header.AceSize)
)
__leave;

// Add the second ACE to the window station.

pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
pace->Mask            = WINSTA_ALL;

if (!AddAce(
pNewAcl,
ACL_REVISION,
MAXDWORD,
(LPVOID)pace,
pace->Header.AceSize)
)
__leave;

// Set a new DACL for the security descriptor.

if (!SetSecurityDescriptorDacl(
psdNew,
TRUE,
pNewAcl,
FALSE)
)
__leave;

// Set the new security descriptor for the window station.

if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
__leave;

// Indicate success.

bSuccess = TRUE;
}
__finally
{
// Free the allocated buffers.

if (pace != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)pace);

if (pNewAcl != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

if (psd != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

if (psdNew != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
}

return bSuccess;

}

BOOL AddAceToDesktop(HDESK hdesk, PSID psid)
{
ACL_SIZE_INFORMATION aclSizeInfo;
BOOL                 bDaclExist;
BOOL                 bDaclPresent;
BOOL                 bSuccess = FALSE;
DWORD                dwNewAclSize;
DWORD                dwSidSize = 0;
DWORD                dwSdSizeNeeded;
PACL                 pacl;
PACL                 pNewAcl = NULL;
PSECURITY_DESCRIPTOR psd = NULL;
PSECURITY_DESCRIPTOR psdNew = NULL;
PVOID                pTempAce;
SECURITY_INFORMATION si = DACL_SECURITY_INFORMATION;
unsigned int         i;

__try
{
// Obtain the security descriptor for the desktop object.

if (!GetUserObjectSecurity(
hdesk,
&si,
psd,
dwSidSize,
&dwSdSizeNeeded))
{
if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
{
psd = (PSECURITY_DESCRIPTOR)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwSdSizeNeeded );

if (psd == NULL)
__leave;

psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwSdSizeNeeded);

if (psdNew == NULL)
__leave;

dwSidSize = dwSdSizeNeeded;

if (!GetUserObjectSecurity(
hdesk,
&si,
psd,
dwSidSize,
&dwSdSizeNeeded)
)
__leave;
}
else
__leave;
}

// Create a new security descriptor.

if (!InitializeSecurityDescriptor(
psdNew,
SECURITY_DESCRIPTOR_REVISION)
)
__leave;

// Obtain the DACL from the security descriptor.

if (!GetSecurityDescriptorDacl(
psd,
&bDaclPresent,
&pacl,
&bDaclExist)
)
__leave;

// Initialize.

ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
aclSizeInfo.AclBytesInUse = sizeof(ACL);

// Call only if NULL DACL.

if (pacl != NULL)
{
// Determine the size of the ACL information.

if (!GetAclInformation(
pacl,
(LPVOID)&aclSizeInfo,
sizeof(ACL_SIZE_INFORMATION),
AclSizeInformation)
)
__leave;
}

// Compute the size of the new ACL.

dwNewAclSize = aclSizeInfo.AclBytesInUse +
sizeof(ACCESS_ALLOWED_ACE) +
GetLengthSid(psid) - sizeof(DWORD);

// Allocate buffer for the new ACL.

pNewAcl = (PACL)HeapAlloc(
GetProcessHeap(),
HEAP_ZERO_MEMORY,
dwNewAclSize);

if (pNewAcl == NULL)
__leave;

// Initialize the new ACL.

if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
__leave;

// If DACL is present, copy it to a new DACL.

if (bDaclPresent)
{
// Copy the ACEs to the new ACL.
if (aclSizeInfo.AceCount)
{
for (i=0; i < aclSizeInfo.AceCount; i++)
{
// Get an ACE.
if (!GetAce(pacl, i, &pTempAce))
__leave;

// Add the ACE to the new ACL.
if (!AddAce(
pNewAcl,
ACL_REVISION,
MAXDWORD,
pTempAce,
((PACE_HEADER)pTempAce)->AceSize)
)
__leave;
}
}
}

// Add ACE to the DACL.

if (!AddAccessAllowedAce(
pNewAcl,
ACL_REVISION,
DESKTOP_ALL,
psid)
)
__leave;

// Set new DACL to the new security descriptor.

if (!SetSecurityDescriptorDacl(
psdNew,
TRUE,
pNewAcl,
FALSE)
)
__leave;

// Set the new security descriptor for the desktop object.

if (!SetUserObjectSecurity(hdesk, &si, psdNew))
__leave;

// Indicate success.

bSuccess = TRUE;
}
__finally
{
// Free buffers.

if (pNewAcl != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)pNewAcl);

if (psd != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)psd);

if (psdNew != NULL)
HeapFree(GetProcessHeap(), 0, (LPVOID)psdNew);
}

return bSuccess;
}

*/