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
			wchar_t uname[DEFAULT_BUFSIZE];
			MultiByteToWideChar(CP_ACP, 0, UserName, -1, uname, DEFAULT_BUFSIZE);

			NET_DISPLAY_USER *users = (NET_DISPLAY_USER *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
				if (!wcscmp(users[i].usri1_name, uname))
					res = TRUE;
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
			wchar_t gname[DEFAULT_BUFSIZE];
			MultiByteToWideChar(CP_ACP, 0, GroupName, -1, gname, DEFAULT_BUFSIZE);

			NET_DISPLAY_GROUP *groups = (NET_DISPLAY_GROUP *)SortedBuffer;
			for (DWORD i = 0; (FALSE == res) && (i < ReturnedEntryCount); i++)
				if (!wcscmp(groups[i].grpi3_name, gname))
					res = TRUE;
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
						(0 != strcmp(Name, "GameCtrl")) &&
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



BOOL SetFileDACL(const char* file, PSECURITY_DESCRIPTOR psec)
{
	if ((NULL == file) || (NULL == psec))
		return NULL;

	SECURITY_INFORMATION RequestedInformation = DACL_SECURITY_INFORMATION;
	BOOL sec = SetFileSecurity(file, RequestedInformation, psec);

	if (FALSE == sec)
		CheckError("Impossible d'installer le nouvel ACL", ::GetLastError());

	return sec;
}


PSECURITY_DESCRIPTOR SetSecurity(PSECURITY_DESCRIPTOR psec)
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
				else                                // FILE_GENERIC_EXECUTE =>
				{									//		STANDARD_RIGHTS_EXECUTE + FILE_EXECUTE + FILE_READ_ATTRIBUTES + SYNCHRONIZE;
					BOOL bAddAce = FALSE;
					if (TRUE == IsSIDAdmin(account))
						bAddAce = AddAce(pNewAcl, ACL_REVISION, MAXDWORD, ace, header->AceSize);
					else if (FILE_GENERIC_EXECUTE == (access & FILE_GENERIC_EXECUTE))
					{
						if (0 != strcmp(Name, "GameCtrl"))
							allowed->Mask = access & ~FILE_GENERIC_EXECUTE;
						else
						{
							bGameCtrlFound = TRUE;
						}
						bAddAce = AddAce(pNewAcl, ACL_REVISION, MAXDWORD, ace, header->AceSize);
					}
					else
						bAddAce = AddAce(pNewAcl, ACL_REVISION, MAXDWORD, ace, header->AceSize);

					if (FALSE == bAddAce)
						CheckError("Ajout ACE impossible", ::GetLastError());
				}
				
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

	// Initialize a security descriptor.
	
	PSECURITY_DESCRIPTOR pNewSD = (PSECURITY_DESCRIPTOR)malloc(2048);
	if (NULL == pNewSD)
		CheckError("Impossible de créer un descripteur de sécurité", ::GetLastError());

	if (!InitializeSecurityDescriptor(pNewSD,SECURITY_DESCRIPTOR_REVISION))
		CheckError("Impossible d'initialiser un descripteur de sécurité", ::GetLastError());
	
	

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
		sec = MakeAbsoluteSD(psec,
							 pNewSD,
							 &dwAbsoluteSecurityDescriptorSize,
							 pDacl,
							 &dwDaclSize,
							 pSacl,
							 &dwSaclSize,
							 pOwner,
							 &dwOwnerSize,
							 pPrimaryGroup,
							 &dwPrimaryGroupSize);

		sec = IsValidSecurityDescriptor(pNewSD);
	}

	// Upadte DACL of the converted security descriptor.
	sec = SetSecurityDescriptorDacl(pNewSD, TRUE, pNewAcl, FALSE);
	if (FALSE == sec)
	{
		CheckError("Impossible d'installer le nouvel ACL", ::GetLastError());
		free(pNewSD);
		pNewSD = NULL;
	}

	return pNewSD;
}
