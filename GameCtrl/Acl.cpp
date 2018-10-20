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

	wchar_t uname[DEFAULT_BUFSIZE];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, DEFAULT_BUFSIZE);
	wchar_t pwd[DEFAULT_BUFSIZE];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, pwd, DEFAULT_BUFSIZE);
	ui.usri1_name = uname;
	ui.usri1_password = pwd;
	ui.usri1_priv = USER_PRIV_USER;
	ui.usri1_flags = UF_NORMAL_ACCOUNT | UF_SCRIPT;


	DWORD dwError = -1;
	NET_API_STATUS status = NetUserAdd(NULL, 1, (LPBYTE)&ui, &dwError);

	BOOL res = FALSE;
	switch (status)
	{
		case ERROR_ACCESS_DENIED:
			Error(IDS_NOPRIVILEGE);
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

			char users[DEFAULT_BUFSIZE];
			DWORD sidSize = DEFAULT_BUFSIZE;
			if (FALSE == CreateWellKnownSid(WinBuiltinUsersSid, NULL, &users, &sidSize))
			{
				CheckError("Impossible de créer le SID utilisateurs", ::GetLastError());
				return FALSE;
			}

			TCHAR GroupName[DEFAULT_BUFSIZE];
			TCHAR DomainName[DEFAULT_BUFSIZE];
			DWORD bufferSize = DEFAULT_BUFSIZE;
			SID_NAME_USE use = SidTypeGroup;
			if (FALSE == LookupAccountSid(NULL, &users, GroupName, &bufferSize, DomainName, &bufferSize, &use))
			{
				MessageBox(NULL, "Impossible de trouver le nom du groupe", "Erreur", MB_OK);
			}
			else
			{
				char buffer[DEFAULT_BUFSIZE];
				sprintf_s(buffer, "Nom du groupe: %s", GroupName);
				MessageBox(NULL, buffer, "Info", MB_OK);
			}

			wchar_t gname[DEFAULT_BUFSIZE];
			MultiByteToWideChar(CP_ACP, 0, GroupName, -1, gname, DEFAULT_BUFSIZE);
			LOCALGROUP_MEMBERS_INFO_3 ginfo;
			ginfo.lgrmi3_domainandname = uname;
			status = NetLocalGroupAddMembers(NULL,	// LPCWSTR  ServerName, NULL is local
											 gname,
											 3,		// DWORD   level,
											 (LPBYTE)&ginfo,
											 1);
			res = (NERR_Success == status);

			if (FALSE == res)
			{
				char buffer[DEFAULT_BUFSIZE];
				sprintf_s(buffer, "AddGroupMember: erreur %d", status);
				MessageBox(NULL, buffer, "Erreur inconnue", MB_OK);
			}

			break;
		}
		default:
		{
			char buffer[DEFAULT_BUFSIZE];
			sprintf_s(buffer, "CreateUser: erreur inconnue %d", status);
			MessageBox(NULL, buffer, "Erreur inconnue", MB_OK);
		}
	}

	return res;
}

BOOL DeleteUser(const char* UserName)
{
	wchar_t uname[DEFAULT_BUFSIZE];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, DEFAULT_BUFSIZE);

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
				char buffer[DEFAULT_BUFSIZE];
				int size = 0;
				WideCharToMultiByte(CP_ACP, 0, users[i].usri1_name, -1, buffer, DEFAULT_BUFSIZE, NULL, NULL);
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
			Error(IDS_NOPRIVILEGE);
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
				char buffer[DEFAULT_BUFSIZE];
				int size = 0;
				WideCharToMultiByte(CP_ACP, 0, groups[i].grpi3_name, -1, buffer, DEFAULT_BUFSIZE, NULL, NULL);
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


BOOL SetSecurity(const char* file)
{
	//ATTRIBUTE_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	//BACKUP_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	SECURITY_INFORMATION RequestedInformation = DACL_SECURITY_INFORMATION;
	
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	unsigned char buffer[DEFAULT_BUFSIZE];
	DWORD         nLength = DEFAULT_BUFSIZE;
	DWORD         nLengthNeeded = 0;
	BOOL sec = GetFileSecurity(file, RequestedInformation, buffer, nLength, &nLengthNeeded);
	if (TRUE != sec)
	{
		CheckError(file, ::GetLastError());
		return FALSE;
	}


	pSecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

	SECURITY_DESCRIPTOR_CONTROL pControl;
	DWORD dwRevision = 0;
	GetSecurityDescriptorControl(pSecurityDescriptor, &pControl, &dwRevision);
	switch (pControl)
	{
		case SE_OWNER_DEFAULTED: break;	// (0x0001)
		case SE_GROUP_DEFAULTED: break; // (0x0002)
		case SE_DACL_PRESENT: break; // (0x0004)
		case SE_DACL_DEFAULTED: break; // (0x0008)
		case SE_SACL_PRESENT: break; // (0x0010)
		case SE_SACL_DEFAULTED: break; // (0x0020)
		case SE_DACL_AUTO_INHERIT_REQ: break; // (0x0100)
		case SE_SACL_AUTO_INHERIT_REQ: break; // (0x0200)
		case SE_DACL_AUTO_INHERITED: break; // (0x0400)
		case SE_SACL_AUTO_INHERITED: break; // (0x0800)
		case SE_DACL_PROTECTED: break; // (0x1000)
		case SE_SACL_PROTECTED: break; // (0x2000)
		case SE_RM_CONTROL_VALID: break; // (0x4000)
		case SE_SELF_RELATIVE: break; // (0x8000)
	}

	PACL dacl = NULL;
	BOOL DaclPresent = FALSE;
	BOOL DaclDefaulted = FALSE;
	GetSecurityDescriptorDacl(pSecurityDescriptor, &DaclPresent, &dacl, &DaclDefaulted);
	for (int j = 0; j < dacl->AceCount; j++)
	{
		LPVOID ace;
		sec = GetAce(dacl, j, &ace);
		if (TRUE != sec)
		{
			CheckError("Impossible d'obtenir les ACL",::GetLastError());
			continue;
		}

		ACE_HEADER *header = (ACE_HEADER*)ace;
		switch (header->AceFlags)
		{
			case OBJECT_INHERIT_ACE: break;
			case CONTAINER_INHERIT_ACE: break;
			case NO_PROPAGATE_INHERIT_ACE: break;
			case INHERIT_ONLY_ACE: break;
			case INHERITED_ACE: break;
			case SUCCESSFUL_ACCESS_ACE_FLAG: break;
			case FAILED_ACCESS_ACE_FLAG: break;
		}

		switch (header->AceType)
		{
			case ACCESS_ALLOWED_ACE_TYPE:
			{
				ACCESS_ALLOWED_ACE *allowed = (ACCESS_ALLOWED_ACE*)ace;
				DWORD sid = allowed->SidStart;
				DWORD access = allowed->Mask;

				PSID account = (PSID)&(allowed->SidStart);
				TCHAR Name[DEFAULT_BUFSIZE];
				TCHAR DomainName[DEFAULT_BUFSIZE];
				DWORD bufferSize = DEFAULT_BUFSIZE;
				SID_NAME_USE use = SidTypeGroup;
				if (FALSE == LookupAccountSid(NULL, account, Name, &bufferSize, DomainName, &bufferSize, &use))
				{
					MessageBox(NULL, "Impossible de trouver le nom du groupe", "Erreur", MB_OK);
				}
				else
				{
					char buffer[DEFAULT_BUFSIZE];
					sprintf_s(buffer, "Nom du compte: %s", Name);
					MessageBox(NULL, buffer, "Info", MB_OK);
				}
				/*
				FILE_GENERIC_EXECUTE =>
					STANDARD_RIGHTS_EXECUTE;
					FILE_EXECUTE;
					FILE_READ_ATTRIBUTES;
					SYNCHRONIZE;
					*/
				break;
			}
			case ACCESS_DENIED_ACE_TYPE:
			{
				ACCESS_DENIED_ACE *denied = (ACCESS_DENIED_ACE*)ace;
				DWORD sid = denied->SidStart;
				DWORD access = denied->Mask;
				break;
			}
			case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
			{
				// Reserved for future use.
				break;
			}
			case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
			{
				ACCESS_ALLOWED_OBJECT_ACE *allowed = (ACCESS_ALLOWED_OBJECT_ACE*)ace;
				DWORD access = allowed->Mask;
				DWORD sid = allowed->SidStart;
				DWORD flags = allowed->Flags;
				GUID ot = allowed->ObjectType;
				GUID iot = allowed->InheritedObjectType;
				break;
			}
			case ACCESS_DENIED_OBJECT_ACE_TYPE:
			{
				ACCESS_DENIED_OBJECT_ACE *denied = (ACCESS_DENIED_OBJECT_ACE*)ace;
				DWORD access = denied->Mask;
				DWORD sid = denied->SidStart;
				DWORD flags = denied->Flags;
				GUID ot = denied->ObjectType;
				GUID iot = denied->InheritedObjectType;
				break;
			}
			case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
			{
				ACCESS_ALLOWED_CALLBACK_ACE *allowed = (ACCESS_ALLOWED_CALLBACK_ACE*)ace;
				DWORD sid = allowed->SidStart;
				DWORD access = allowed->Mask;
				break;
			}
			case ACCESS_DENIED_CALLBACK_ACE_TYPE:
			{
				ACCESS_DENIED_CALLBACK_ACE *denied = (ACCESS_DENIED_CALLBACK_ACE*)ace;
				DWORD sid = denied->SidStart;
				DWORD access = denied->Mask;
				break;
			}
			case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
			{
				ACCESS_ALLOWED_CALLBACK_OBJECT_ACE *allowed = (ACCESS_ALLOWED_CALLBACK_OBJECT_ACE*)ace;
				DWORD access = allowed->Mask;
				DWORD sid = allowed->SidStart;
				DWORD flags = allowed->Flags;
				GUID ot = allowed->ObjectType;
				GUID iot = allowed->InheritedObjectType;
				break;
			}
			case ACCESS_DENIED_CALLBACK_OBJECT_ACE_TYPE:
			{
				ACCESS_DENIED_CALLBACK_OBJECT_ACE *denied = (ACCESS_DENIED_CALLBACK_OBJECT_ACE*)ace;
				DWORD access = denied->Mask;
				DWORD sid = denied->SidStart;
				DWORD flags = denied->Flags;
				GUID ot = denied->ObjectType;
				GUID iot = denied->InheritedObjectType;
				break;
			}
		}
	}

	return TRUE;
}



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


