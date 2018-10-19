// Acl.cpp : manages user rights and security.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>
#include <lm.h>


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


BOOL CreateUser(const char* UserName)
{
	USER_INFO_1  ui;
	ZeroMemory(&ui, sizeof(ui));

	wchar_t uname[256];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, 256);
	wchar_t pwd[256];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, pwd, 256);
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
			CreateWellKnownSid(WinBuiltinUsersSid, NULL, &users, &sidSize);

			TCHAR GroupName[256];
			TCHAR DomainName[256];
			DWORD bufferSize = 256;
			SID_NAME_USE use = SidTypeGroup;
			LookupAccountSid(NULL, &users, GroupName, &bufferSize, DomainName, &bufferSize, &use);


			wchar_t gname[256];
			MultiByteToWideChar(CP_ACP, 0, GroupName, -1, uname, 256);
			LOCALGROUP_MEMBERS_INFO_0 ginfo;
			ginfo.lgrmi0_sid = 0;
			status = NetLocalGroupAddMembers(NULL,	// LPCWSTR  ServerName, NULL is local
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
					res = TRUE;
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
					res = TRUE;
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


BOOL ExecuteAsAdmin(const char *file, const char *options)
{
	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = hWnd;
	shExInfo.lpVerb = _T("runas");

	// Operation to perform
	shExInfo.lpFile = _T(file);
	// Application to start
	shExInfo.lpParameters = _T(options);
	// Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if (ShellExecuteEx(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
	}

	return TRUE;
}


#define DESKTOP_ALL (DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | \
		DESKTOP_CREATEMENU | DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | \
		DESKTOP_JOURNALPLAYBACK | DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | \
		DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL (WINSTA_ENUMDESKTOPS | WINSTA_READATTRIBUTES | \
		WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | \
		WINSTA_WRITEATTRIBUTES | WINSTA_ACCESSGLOBALATOMS | \
		WINSTA_EXITWINDOWS | WINSTA_ENUMERATE | WINSTA_READSCREEN | \
		STANDARD_RIGHTS_REQUIRED)

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)


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
		if (!GetUserObjectSecurity(hwinsta,
			&si,
			psd,
			dwSidSize,
			&dwSdSizeNeeded))
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);

				if (psd == NULL)
					__leave;

				psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);

				if (psdNew == NULL)
					__leave;

				dwSidSize = dwSdSizeNeeded;

				if (!GetUserObjectSecurity(hwinsta,
					&si,
					psd,
					dwSidSize,
					&dwSdSizeNeeded))
					__leave;
			}
			else
				__leave;

		// Create a new DACL.
		if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
			__leave;

		// Get the DACL from the security descriptor.
		if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
			__leave;

		// Initialize the ACL.
		ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
		aclSizeInfo.AclBytesInUse = sizeof(ACL);

		// Call only if the DACL is not NULL.
		if (pacl != NULL)
		{
			// get the file ACL size info
			if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
				__leave;
		}

		// Compute the size of the new ACL.
		dwNewAclSize = aclSizeInfo.AclBytesInUse +
			(2 * sizeof(ACCESS_ALLOWED_ACE)) + (2 * GetLengthSid(psid)) -
			(2 * sizeof(DWORD));

		// Allocate memory for the new ACL.
		pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);

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
				for (i = 0; i < aclSizeInfo.AceCount; i++)
				{
					// Get an ACE.
					if (!GetAce(pacl, i, &pTempAce))
						__leave;

					// Add the ACE to the new ACL.
					if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
						__leave;
				}
			}
		}

		// Add the first ACE to the window station.
		pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(GetProcessHeap(),
											   HEAP_ZERO_MEMORY,
											   sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) -
											   sizeof(DWORD));

		if (pace == NULL)
			__leave;

		pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
		pace->Header.AceFlags = CONTAINER_INHERIT_ACE |
			INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
		pace->Header.AceSize = LOWORD(sizeof(ACCESS_ALLOWED_ACE) +
									  GetLengthSid(psid) - sizeof(DWORD));
		pace->Mask = GENERIC_ACCESS;

		if (!CopySid(GetLengthSid(psid), &pace->SidStart, psid))
			__leave;

		if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
			__leave;

		// Add the second ACE to the window station.
		pace->Header.AceFlags = NO_PROPAGATE_INHERIT_ACE;
		pace->Mask = WINSTA_ALL;

		if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, (LPVOID)pace, pace->Header.AceSize))
			__leave;

		// Set a new DACL for the security descriptor.
		if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
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

		if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);

				if (psd == NULL)
					__leave;

				psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);

				if (psdNew == NULL)
					__leave;

				dwSidSize = dwSdSizeNeeded;

				if (!GetUserObjectSecurity(hdesk, &si, psd, dwSidSize, &dwSdSizeNeeded))
					__leave;
			}
			else
				__leave;
		}

		// Create a new security descriptor.

		if (!InitializeSecurityDescriptor(psdNew, SECURITY_DESCRIPTOR_REVISION))
			__leave;

		// Obtain the DACL from the security descriptor.
		if (!GetSecurityDescriptorDacl(psd, &bDaclPresent, &pacl, &bDaclExist))
			__leave;

		// Initialize.
		ZeroMemory(&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION));
		aclSizeInfo.AclBytesInUse = sizeof(ACL);

		// Call only if NULL DACL.

		if (pacl != NULL)
		{
			// Determine the size of the ACL information.
			if (!GetAclInformation(pacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
				__leave;
		}

		// Compute the size of the new ACL.
		dwNewAclSize = aclSizeInfo.AclBytesInUse + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

		// Allocate buffer for the new ACL.
		pNewAcl = (PACL)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwNewAclSize);

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
				for (i = 0; i < aclSizeInfo.AceCount; i++)
				{
					// Get an ACE.
					if (!GetAce(pacl, i, &pTempAce))
						__leave;

					// Add the ACE to the new ACL.
					if (!AddAce(pNewAcl, ACL_REVISION, MAXDWORD, pTempAce, ((PACE_HEADER)pTempAce)->AceSize))
						__leave;
				}
			}
		}

		// Add ACE to the DACL.
		if (!AddAccessAllowedAce(pNewAcl, ACL_REVISION, DESKTOP_ALL, psid))
			__leave;

		// Set new DACL to the new security descriptor.
		if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
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

