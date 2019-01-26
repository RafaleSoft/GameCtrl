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
			b = FALSE;
		FreeSid(AdministratorsGroup);
	}

	return(b);
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


BOOL IsSIDAdmin(PSID psid)
{
	//	Check SID first
	if (!IsValidSid(psid))
		return FALSE;

	SID sid = *(SID*)psid;

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID_IDENTIFIER_AUTHORITY auth = GetSidIdentifierAuthority(psid);
	if (0 != memcmp(auth, &NtAuthority, sizeof(SID_IDENTIFIER_AUTHORITY)))
		return FALSE;

	PUCHAR count = GetSidSubAuthorityCount(psid);
	if (NULL == count)
		return FALSE;

	BOOL b = TRUE;
	for (UCHAR sa = 0; (sa < *count) && (TRUE == b); sa++)
	{
		PDWORD subAuth = GetSidSubAuthority(psid, sa);
		if (NULL == subAuth)
			b = FALSE;
		else
			b = b && ((SECURITY_AUTHENTICATED_USER_RID == *subAuth) ||
					  (SECURITY_LOCAL_SYSTEM_RID == *subAuth) ||
					  (SECURITY_BUILTIN_DOMAIN_RID == *subAuth) ||
					  (DOMAIN_USER_RID_ADMIN == *subAuth) ||
					  (DOMAIN_GROUP_RID_ADMINS == *subAuth) ||
					  (DOMAIN_ALIAS_RID_ADMINS == *subAuth));
	}

	return b;
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



BOOL CreateUser(const char* UserName, const char* PassWord)
{
	USER_INFO_1  ui;
	ZeroMemory(&ui, sizeof(ui));

	wchar_t uname[DEFAULT_BUFSIZE];
	MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, DEFAULT_BUFSIZE);
	wchar_t pwd[DEFAULT_BUFSIZE];
	MultiByteToWideChar(CP_ACP, 0, PassWord, -1, pwd, DEFAULT_BUFSIZE);
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

			wchar_t *gname = toWchar(GroupName);
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

			delete[] gname;
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
	wchar_t *uname = toWchar(UserName);
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
			//MessageBox(NULL, "Utilisateur inconnu", "Erreur de login", MB_OK);
			// Nothing done here: if user is not found, 
			// assume uninstall is in progress, or has been called & unfinished.
			// Hence, return true here.
			res = TRUE;
			break;
		case NERR_Success:
		{
			res = TRUE;
			break;
		}
	}

	delete[] uname;
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
			wchar_t *uname = toWchar(UserName);

			NET_DISPLAY_USER *users = (NET_DISPLAY_USER *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
				if (!wcscmp(users[i].usri1_name, uname))
					res = TRUE;
			break;

			delete[] uname;
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
			wchar_t *gname = toWchar(GroupName);

			NET_DISPLAY_GROUP *groups = (NET_DISPLAY_GROUP *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
				if (!wcscmp(groups[i].grpi3_name, gname))
					res = TRUE;
			delete[] gname;
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

	BOOL res = TRUE;
	if (ShellExecuteEx(&shExInfo))
	{
		if (0 != shExInfo.hProcess)
		{
			WaitForSingleObject(shExInfo.hProcess, INFINITE);
			CloseHandle(shExInfo.hProcess);
		}
		else
		{
			CheckError("Unable to run GameCtrl as Administrator", GetLastError());
			res = FALSE;
		}
	}

	return res;
}



PSECURITY_DESCRIPTOR GetFileDACL(const char* file)
{
	if (NULL == file)
		return NULL;

	//ATTRIBUTE_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	//BACKUP_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	SECURITY_INFORMATION RequestedInformation = DACL_SECURITY_INFORMATION;

	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	DWORD         nLengthNeeded = 0;
	unsigned char *buffer = NULL;

	BOOL sec = GetFileSecurity(file, RequestedInformation, buffer, 0, &nLengthNeeded);
	if (FALSE == sec)
	{
		if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			buffer = new unsigned char[nLengthNeeded];
			if (NULL != buffer)
			{
				DWORD dwLength = nLengthNeeded;
				sec = GetFileSecurity(file, RequestedInformation, buffer, dwLength, &nLengthNeeded);
			}
		}
	}
	if (FALSE == sec)
	{
		if (NULL != buffer)
			delete[] buffer;
		CheckError(file, ::GetLastError());
		return NULL;
	}

	pSecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

	SECURITY_DESCRIPTOR_CONTROL pControl;
	DWORD dwRevision = 0;
	GetSecurityDescriptorControl(pSecurityDescriptor, &pControl, &dwRevision);
	if (SE_DACL_PRESENT != (pControl & SE_DACL_PRESENT))
	{
		if (NULL != buffer)
			delete[] buffer;
		return NULL;
	}

	return pSecurityDescriptor;
}


BOOL CheckSecurity(const char* file)
{
	PSECURITY_DESCRIPTOR psec = GetFileDACL(file);
	if (NULL == psec)
		return NULL;

	PACL dacl = NULL;
	BOOL DaclPresent = FALSE;
	BOOL DaclDefaulted = FALSE;
	BOOL sec = GetSecurityDescriptorDacl(psec, &DaclPresent, &dacl, &DaclDefaulted);
	
	for (DWORD j = 0; (j < dacl->AceCount) && (TRUE == sec); j++)
	{
		LPVOID ace = NULL;
		sec = sec && GetAce(dacl, j, &ace);
		if ((TRUE != sec) || (NULL == ace))
		{
			CheckError("Impossible d'obtenir les ACL", ::GetLastError());
			break;
		}

		ACE_HEADER *header = (ACE_HEADER*)ace;
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
					Error(IDS_GROUPNOTFOUND);
				else
				{
					if ((FALSE == IsSIDAdmin(account)) &&
						(0 != strcmp(Name, USER_NAME)) &&
						(FILE_GENERIC_EXECUTE == (access & FILE_GENERIC_EXECUTE)))
					{
						//char buffer[DEFAULT_BUFSIZE];
						//sprintf_s(buffer, "Droits incorrects pour le compte: %s", Name);
						//MessageBox(NULL, buffer, "Info", MB_OK);
						sec = FALSE;
					}
				}
				break;
			}
			default:
				sec = FALSE;
				break;
		}
	}

	delete[] psec;
	return sec;
}



BOOL SetFileDACL(const char* file, PSECURITY_DESCRIPTOR psec, PACL pNewAcl)
{
	if ((NULL == file) || (NULL == psec))
		return NULL;

	// Initialize a new security descriptor.
	PSECURITY_DESCRIPTOR pNewSD = (PSECURITY_DESCRIPTOR)malloc(2048);
	if (NULL == pNewSD)
	{
		CheckError("Impossible de créer un descripteur de sécurité", ::GetLastError());
		return FALSE;
	}

	if (!InitializeSecurityDescriptor(pNewSD, SECURITY_DESCRIPTOR_REVISION))
	{
		CheckError("Impossible d'initialiser un descripteur de sécurité", ::GetLastError());
		free(pNewSD);
		return FALSE;
	}

	BOOL sec = FALSE;
	SECURITY_DESCRIPTOR_CONTROL pControl;
	DWORD dwRevision = 0;

	GetSecurityDescriptorControl(psec, &pControl, &dwRevision);
	if (SE_SELF_RELATIVE == (pControl & SE_SELF_RELATIVE))
	{
		PACL pDacl = (PACL)malloc(256);
		DWORD dwDaclSize = 256;
		PACL pSacl = (PACL)malloc(256);
		DWORD dwSaclSize = 256;
		PSID pOwner = (PSID)malloc(256);
		DWORD dwOwnerSize = 256;
		PSID pPrimaryGroup = (PSID)malloc(256);
		DWORD dwPrimaryGroupSize = 256;

		DWORD dwAbsoluteSecurityDescriptorSize = 2048;
		sec = MakeAbsoluteSD(psec, pNewSD, &dwAbsoluteSecurityDescriptorSize,
							 pDacl, &dwDaclSize,
							 pSacl, &dwSaclSize,
							 pOwner, &dwOwnerSize,
							 pPrimaryGroup, &dwPrimaryGroupSize);

#pragma warning(suppress: 6102)
		sec = IsValidSecurityDescriptor(pNewSD);
	}

	// Upadte DACL of the converted security descriptor.
	sec = SetSecurityDescriptorDacl(pNewSD, TRUE, pNewAcl, FALSE);
	if (FALSE == sec)
		CheckError("Impossible d'installer le nouvel ACL", ::GetLastError());
	else
	{
		sec = SetFileSecurity(file, DACL_SECURITY_INFORMATION, pNewSD);
		if (FALSE == sec)
			CheckError("Impossible d'installer le nouvel ACL", ::GetLastError());
	}

	free(pNewSD);
	return sec;
}


PACL SetSecurity(PSECURITY_DESCRIPTOR psec)
{
	PACL dacl = NULL;
	BOOL DaclPresent = FALSE;
	BOOL DaclDefaulted = FALSE;
	BOOL sec = GetSecurityDescriptorDacl(psec, &DaclPresent, &dacl, &DaclDefaulted);
	
	ACL_SIZE_INFORMATION aclSizeInfo;
	memset(&aclSizeInfo, 0, sizeof(ACL_SIZE_INFORMATION));
	sec = GetAclInformation(dacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);
	if (FALSE == sec)
	{
		CheckError("Impossible d'obtenir les informations sur ACL", ::GetLastError());
		return NULL;
	}

	// Obtain the GameCtrlUser Sid
	TCHAR sidbuffer[DEFAULT_BUFSIZE];
	DWORD cbSid = DEFAULT_BUFSIZE;
	TCHAR ReferencedDomainName[DEFAULT_BUFSIZE];
	DWORD cchReferencedDomainName = DEFAULT_BUFSIZE;
	SID_NAME_USE peUse;
	if (FALSE == LookupAccountName(NULL, USER_NAME, sidbuffer, &cbSid, ReferencedDomainName, &cchReferencedDomainName, &peUse))
	{
		CheckError("Compte de jeu non installé", ::GetLastError());
		return NULL;
	}
	PSID psid = (PSID)sidbuffer;
		
	// Compute the size of the new ACL.
	DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse +
							sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD);

	// Allocate memory for the new ACL.
	unsigned char *aclBuffer = new unsigned char[dwNewAclSize];
	PACL pNewAcl = (PACL)aclBuffer;
	if (pNewAcl == NULL)
		return NULL;

	// Initialize the new DACL.
	if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
	{
		CheckError("Impossible d'initialiser le nouvel ACL", ::GetLastError());
		delete[] aclBuffer;
		return NULL;
	}

	BOOL bGameCtrlFound = FALSE;
	sec = TRUE;
	for (DWORD j = 0; (j < aclSizeInfo.AceCount) && (TRUE == sec); j++)
	{
		LPVOID ace;
		sec = GetAce(dacl, j, &ace);
		if (TRUE != sec)
		{
			CheckError("Impossible d'obtenir les ACL",::GetLastError());
			continue;
		}

		ACE_HEADER *header = (ACE_HEADER*)ace;
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
					Error(IDS_GROUPNOTFOUND);
				else if (use == SidTypeUser)        // FILE_GENERIC_EXECUTE =>
				{									//		STANDARD_RIGHTS_EXECUTE + FILE_EXECUTE + FILE_READ_ATTRIBUTES + SYNCHRONIZE;
					if ((FALSE == IsSIDAdmin(account)) &&
						(FILE_GENERIC_EXECUTE == (access & FILE_GENERIC_EXECUTE)))
					{
						if (0 != strcmp(Name, USER_NAME))
							allowed->Mask = access & ~FILE_EXECUTE;
						else	// Ensure GameCtrl will be able to execute the game
						{
							allowed->Mask = access | FILE_GENERIC_EXECUTE;
							bGameCtrlFound = TRUE;
						}
					}
				}
				else if ((use == SidTypeAlias) && (TRUE == IsWellKnownSid(account, WinBuiltinUsersSid)))
					allowed->Mask = access & ~FILE_EXECUTE;
				else if ((use == SidTypeWellKnownGroup) && (TRUE == IsWellKnownSid(account, WinAuthenticatedUserSid)))
					allowed->Mask = access & ~FILE_EXECUTE;
				else if ((use == SidTypeWellKnownGroup) && (TRUE == IsWellKnownSid(account, WinWorldSid)))
					allowed->Mask = access & ~FILE_EXECUTE;

				BOOL bAddAce = AddAce(pNewAcl, ACL_REVISION, MAXDWORD, ace, header->AceSize);
				if (FALSE == bAddAce)
					CheckError("Ajout ACE impossible", ::GetLastError());

				break;
			}
			default:
				break;
		}
	}

	if (FALSE == bGameCtrlFound)
	{
		sec = AddAccessAllowedAce(pNewAcl, ACL_REVISION, FILE_ALL_ACCESS, psid);
		if (FALSE == sec)
			CheckError("Impossible d'ajouter le compte de jeu", ::GetLastError());
	}
	
	sec = IsValidAcl(pNewAcl);
	if (FALSE == sec)
	{
		CheckError("ACL invalide", ::GetLastError());
		delete[] aclBuffer;
		pNewAcl = FALSE;
	}

	return pNewAcl;
}


PACL UnsetSecurity(PSECURITY_DESCRIPTOR psec)
{
	PACL dacl = NULL;
	BOOL DaclPresent = FALSE;
	BOOL DaclDefaulted = FALSE;
	BOOL sec = GetSecurityDescriptorDacl(psec, &DaclPresent, &dacl, &DaclDefaulted);

	ACL_SIZE_INFORMATION aclSizeInfo;
	memset(&aclSizeInfo, 0, sizeof(ACL_SIZE_INFORMATION));
	sec = GetAclInformation(dacl, (LPVOID)&aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);
	if (FALSE == sec)
	{
		CheckError("Impossible d'obtenir les informations sur ACL", ::GetLastError());
		return NULL;
	}

	// Obtain the GameCtrlUser Sid
	TCHAR sidbuffer[DEFAULT_BUFSIZE];
	DWORD cbSid = DEFAULT_BUFSIZE;
	TCHAR ReferencedDomainName[DEFAULT_BUFSIZE];
	DWORD cchReferencedDomainName = DEFAULT_BUFSIZE;
	SID_NAME_USE peUse;
	if (FALSE == LookupAccountName(NULL, "GameCtrl", sidbuffer, &cbSid, ReferencedDomainName, &cchReferencedDomainName, &peUse))
	{
		CheckError("Compte de jeu non installé", ::GetLastError());
		return NULL;
	}
	PSID psid = (PSID)sidbuffer;

	// Compute the size of the new ACL (remove GameCtrl access)
	DWORD dwNewAclSize = aclSizeInfo.AclBytesInUse -
		sizeof(ACCESS_ALLOWED_ACE) - GetLengthSid(psid) + sizeof(DWORD);

	// Allocate memory for the new ACL.
	unsigned char *aclBuffer = new unsigned char[dwNewAclSize];
	PACL pNewAcl = (PACL)aclBuffer;
	if (pNewAcl == NULL)
		return NULL;

	// Initialize the new DACL.
	if (!InitializeAcl(pNewAcl, dwNewAclSize, ACL_REVISION))
	{
		CheckError("Impossible d'initialiser le nouvel ACL", ::GetLastError());
		delete[] aclBuffer;
		return NULL;
	}

	sec = TRUE;
	for (DWORD j = 0; (j < aclSizeInfo.AceCount) && (TRUE == sec); j++)
	{
		LPVOID ace;
		sec = GetAce(dacl, j, &ace);
		if (TRUE != sec)
		{
			CheckError("Impossible d'obtenir les ACL:", ::GetLastError());
			continue;
		}

		ACE_HEADER *header = (ACE_HEADER*)ace;
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
					Error(IDS_GROUPNOTFOUND);
				else if (use == SidTypeUser)        // FILE_GENERIC_EXECUTE =>
				{									//		STANDARD_RIGHTS_EXECUTE + FILE_EXECUTE + FILE_READ_ATTRIBUTES + SYNCHRONIZE;
					if ((FALSE == IsSIDAdmin(account)) &&
						(FILE_GENERIC_EXECUTE == (access & FILE_GENERIC_EXECUTE)))
					{
						if (0 != strcmp(Name, "GameCtrl"))
							allowed->Mask = access & FILE_EXECUTE;
						else	// Ensure GameCtrl will be able to execute the game
						{
							// Do nothing : GameCtrl is being removed.
						}
					}
				}
				else
				{
					if ((use == SidTypeAlias) && (TRUE == IsWellKnownSid(account, WinBuiltinUsersSid)))
						allowed->Mask = access | FILE_EXECUTE;
					else if ((use == SidTypeWellKnownGroup) && (TRUE == IsWellKnownSid(account, WinAuthenticatedUserSid)))
						allowed->Mask = access | FILE_EXECUTE;
					else if ((use == SidTypeWellKnownGroup) && (TRUE == IsWellKnownSid(account, WinWorldSid)))
						allowed->Mask = access | FILE_EXECUTE;

					BOOL bAddAce = AddAce(pNewAcl, ACL_REVISION, MAXDWORD, ace, header->AceSize);
					if (FALSE == bAddAce)
						CheckError("Ajout ACE impossible:", ::GetLastError());
				}
				break;
			}
			default:
				break;
		}
	}

	sec = IsValidAcl(pNewAcl);
	if (FALSE == sec)
	{
		CheckError("ACL invalide:", ::GetLastError());
		delete[] aclBuffer;
		pNewAcl = FALSE;
	}

	return pNewAcl;
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
		if (!GetUserObjectSecurity(hwinsta,&si,psd,dwSidSize,&dwSdSizeNeeded))
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				psd = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
				if (psd == NULL)
					__leave;
			
				psdNew = (PSECURITY_DESCRIPTOR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSdSizeNeeded);
				if (psdNew == NULL)
					__leave;

				dwSidSize = dwSdSizeNeeded;

				if (!GetUserObjectSecurity(hwinsta,&si,psd,dwSidSize,&dwSdSizeNeeded))
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
		dwNewAclSize = aclSizeInfo.AclBytesInUse + (2 * sizeof(ACCESS_ALLOWED_ACE)) + (2 * GetLengthSid(psid)) - (2 * sizeof(DWORD));
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
		pace = (ACCESS_ALLOWED_ACE *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
		if (pace == NULL)
			__leave;
		pace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
		pace->Header.AceFlags = CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE;
		pace->Header.AceSize = LOWORD(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psid) - sizeof(DWORD));
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

		if (NULL != psdNew)
		{
			// Set a new DACL for the security descriptor.
			if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
				__leave;
			// Set the new security descriptor for the window station.
			if (!SetUserObjectSecurity(hwinsta, &si, psdNew))
				__leave;
		}

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

		if (NULL != psdNew)
		{
			// Set new DACL to the new security descriptor.
			if (!SetSecurityDescriptorDacl(psdNew, TRUE, pNewAcl, FALSE))
				__leave;

			// Set the new security descriptor for the desktop object.
			if (!SetUserObjectSecurity(hdesk, &si, psdNew))
				__leave;
		}
		
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

BOOL ObtainSid(HANDLE hToken, PSID *psid)
{
	BOOL                    bSuccess = FALSE; // assume function will fail
	DWORD                   dwIndex;
	DWORD                   dwLength = 0;
	TOKEN_INFORMATION_CLASS tic = TokenGroups;
	PTOKEN_GROUPS           ptg = NULL;

	__try
	{
		// 
		// determine the size of the buffer
		// 
		if (!GetTokenInformation(hToken,tic,(LPVOID)ptg,0,&dwLength))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				ptg = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwLength);
				if (ptg == NULL)
					__leave;
			}
			else
				__leave;
		}

		// 
		// obtain the groups the access token belongs to
		// 
		if (!GetTokenInformation(hToken,tic,(LPVOID)ptg,dwLength,&dwLength))
			__leave;

		// 
		// determine which group is the logon sid
		// 
		for (dwIndex = 0; dwIndex < ptg->GroupCount; dwIndex++)
		{
			if ((ptg->Groups[dwIndex].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID)
			{
				// 
				// determine the length of the sid
				// 
				dwLength = GetLengthSid(ptg->Groups[dwIndex].Sid);

				// 
				// allocate a buffer for the logon sid
				// 
				*psid = (PSID)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwLength);
				if (*psid == NULL)
					__leave;

				// 
				// obtain a copy of the logon sid
				// 
				if (!CopySid(dwLength, *psid, ptg->Groups[dwIndex].Sid))
					__leave;

				// 
				// break out of the loop because the logon sid has been
				// found
				// 
				break;
			}
		}

		// 
		// indicate success
		// 
		bSuccess = TRUE;
	}
	__finally
	{
		// 
		// free the buffer for the token group
		// 
		if (ptg != NULL)
			HeapFree(GetProcessHeap(), 0, (LPVOID)ptg);
	}

	return bSuccess;

}
