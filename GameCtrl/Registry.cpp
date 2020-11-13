// Regsitry.cpp : Defines registry functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>

static const char *KEY_NAME = TEXT("Software\\GameCtrl");
static const HKEY KEY_ROOT = HKEY_LOCAL_MACHINE;

BOOL CleanRegistry()
{
	HKEY hTestKey = 0;
	REGSAM keySAM = DELETE | KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
	LONG res = RegOpenKeyEx(KEY_ROOT, KEY_NAME, 0, keySAM, &hTestKey);

	if (ERROR_SUCCESS == res)
	{
		res = RegDeleteTree(hTestKey, NULL);
		if (ERROR_SUCCESS == res)
		{
			RegCloseKey(hTestKey);
			res = RegDeleteKey(KEY_ROOT, KEY_NAME);
			if (ERROR_SUCCESS == res)
				return TRUE;
			else
			{
				CheckError("Unable to erase GameCtrl data", res);
				return FALSE;
			}
		}
		else
		{
			CheckError("Unable to erase GameCtrl data", res);
			return FALSE;
		}
	}
	else
	{
		CheckError("Unable to access GameCtrl data", res);
		return FALSE;
	}
}

BOOL InitRegistry(GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	REGSAM keySAM = KEY_WRITE;
	DWORD dwDisposition = 0;

	SECURITY_DESCRIPTOR psec;
	SECURITY_ATTRIBUTES sec;
	sec.nLength = sizeof(SECURITY_ATTRIBUTES);
	sec.lpSecurityDescriptor = &psec;
	sec.bInheritHandle = TRUE;

	BOOL bres = InitializeSecurityDescriptor(&psec, SECURITY_DESCRIPTOR_REVISION);
	PACL pacl = SetSecurity(&psec, KEY_ALL_ACCESS);
	bres = SetSecurityDescriptorDacl(&psec, TRUE, pacl, FALSE);

	LONG res = RegCreateKeyEx(KEY_ROOT,
							  KEY_NAME, 0, NULL,
							  REG_OPTION_NON_VOLATILE,
							  KEY_WRITE | KEY_READ,
							  &sec,
							  &hTestKey,
							  &dwDisposition);
	if (ERROR_SUCCESS == res)
	{
		RegCloseKey(hTestKey);
		res = SetRegistryVars(data);
	}
	else
	{
		CheckError("Unable to access GameCtrl data", res);
		res = FALSE;
	}

	return res;
}

BOOL WriteDWORD(HKEY hKey, const char* name, DWORD pvData)
{
	if ((0 == name) || (0 == hKey))
		return FALSE;

	LONG res = RegSetValueEx(hKey, TEXT(name), 0, REG_DWORD, (BYTE*)&pvData, sizeof(DWORD));
	if (ERROR_SUCCESS != res)
	{
		char buffer[DEFAULT_BUFSIZE];
		sprintf_s(buffer, "Unable to write dword value: %s", name);
		CheckError(buffer, res);
		RegCloseKey(hKey);
		return FALSE;
	}

	return TRUE;
}

BOOL ReadDWORD(HKEY hKey, const char* name, DWORD *pvData)
{
	DWORD pdwType = 0;
	DWORD pcbData = sizeof(DWORD);

	BOOL res = RegGetValue(hKey, NULL, TEXT(name), RRF_RT_REG_DWORD, &pdwType, pvData, &pcbData);
	if ((ERROR_SUCCESS != res) || (REG_DWORD != pdwType) || (sizeof(DWORD) != pcbData))
	{
		CheckError("Unable to read Chrono value", res);
		RegCloseKey(hKey);
		return FALSE;
	}

	return TRUE;
}

BOOL GetRegistryVars(GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	LONG res = 0;

	HANDLE token = NULL;
	BOOL logon = LogonUser(USER_NAME, ".", PASSWORD, LOGON32_LOGON_INTERACTIVE /*LOGON32_LOGON_NETWORK*/, LOGON32_PROVIDER_DEFAULT, &token);
	if (TRUE == logon)
		logon = ImpersonateLoggedOnUser(token);	// TODO check impersonation result
	else
	{
		CheckError("Impossible d'accéder aux données de GameCtrl", ERROR_ACCESS_DENIED);
		return FALSE;
	}

	res = RegOpenKeyEx(KEY_ROOT, KEY_NAME, 0, KEY_READ, &hTestKey);
	BOOL result = FALSE;
	
	if (res == ERROR_SUCCESS)
	{
		DWORD pdwType = 0;
		DWORD pvData = 0;
		DWORD pcbData = sizeof(pvData);

		if (FALSE == ReadDWORD(hTestKey, "CHRONO", (DWORD*)&data.CHRONO))
			return FALSE;
		if (FALSE == ReadDWORD(hTestKey, "REINITCHRONO", (DWORD*)&data.ReinitChrono))
			return FALSE;
		if (FALSE == ReadDWORD(hTestKey, "NBDAYSTOREINIT", (DWORD*)&data.NbDaysToReinit))
			return FALSE;
		if (FALSE == ReadDWORD(hTestKey, "LOWDATETIME", (DWORD*)&data.NextUpdateTime.dwLowDateTime))
			return FALSE;
		if (FALSE == ReadDWORD(hTestKey, "HIGHDATETIME", (DWORD*)&data.NextUpdateTime.dwHighDateTime))
			return FALSE;
		if (FALSE == ReadDWORD(hTestKey, "NBGAMES", (DWORD*)&data.NbGames))
			return FALSE;

		if (NULL != data.Games)
		{
			for (long i = 0; i < data.NbGames; i++)
				delete[] data.Games[i];
			delete[] data.Games;
		}

		data.Games = new const char*[data.NbGames];

		result = TRUE;
		for (long i = 0; i < data.NbGames; i++)
		{
			char buffer[32];
			sprintf_s(buffer, 32, "GAME%d", i);
			res = RegGetValue(hTestKey, NULL, TEXT(buffer), RRF_RT_REG_SZ, &pdwType, NULL, &pcbData);
			if (ERROR_SUCCESS == res)
			{
				data.Games[i] = new char[pcbData];
				res = RegGetValue(hTestKey, NULL, TEXT(buffer), RRF_RT_REG_SZ, &pdwType, (PVOID*)(data.Games[i]), &pcbData);
			}
			else
			{
				CheckError("Impossible de lire le nom du jeu dans les données de GameCtrl", res);
				result = FALSE;
			}
		}
		
		pcbData = 24 * sizeof(unsigned char);
		LONG res = RegGetValue(hTestKey, NULL, "SLOTS", RRF_RT_REG_BINARY, &pdwType, (BYTE*)&data.HourSlots[0], &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError("Impossible de lire les crénaux horaires", res);
			result = FALSE;
		}
		
		res = RegCloseKey(hTestKey);
	}
	else
		CheckError("Impossible d'accéder aux données de GameCtrl", res);

	result = result && RevertToSelf();
	result = result && CloseHandle(token);

	return result;
}



BOOL SetRegistryVars(const GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	LONG res = 0;

	HANDLE token = NULL;
	BOOL logon = LogonUser(USER_NAME, ".", PASSWORD, LOGON32_LOGON_INTERACTIVE /*LOGON32_LOGON_NETWORK*/, LOGON32_PROVIDER_DEFAULT, &token);
	if (TRUE == logon)
		logon = ImpersonateLoggedOnUser(token);	// TODO check impersonation result
	else
	{
		CheckError("Impossible d'accéder aux données de GameCtrl", ERROR_ACCESS_DENIED);
		return FALSE;
	}

	res = RegOpenKeyEx(KEY_ROOT, KEY_NAME, 0, KEY_SET_VALUE, &hTestKey);
	BOOL result = FALSE;

	if (res == ERROR_SUCCESS)
	{
		DWORD pvData = data.CHRONO;
		DWORD pcbData = sizeof(pvData);

		if (FALSE == WriteDWORD(hTestKey, "CHRONO", data.CHRONO))
			return FALSE;
		if (FALSE == WriteDWORD(hTestKey, "REINITCHRONO", data.ReinitChrono))
			return FALSE;
		if (FALSE == WriteDWORD(hTestKey, "NBDAYSTOREINIT", data.NbDaysToReinit))
			return FALSE;
		if (FALSE == WriteDWORD(hTestKey, "LOWDATETIME", data.NextUpdateTime.dwLowDateTime))
			return FALSE;
		if (FALSE == WriteDWORD(hTestKey, "HIGHDATETIME", data.NextUpdateTime.dwHighDateTime))
			return FALSE;
		if (FALSE == WriteDWORD(hTestKey, "NBGAMES", data.NbGames))
			return FALSE;
		
		result = TRUE;
		for (long i = 0; i < data.NbGames; i++)
		{
			const char* game = data.Games[i];
			char buffer[32];
			sprintf_s(buffer, 32, "GAME%d", i);
			res = RegSetValueEx(hTestKey, TEXT(buffer), 0, REG_SZ, (BYTE*)game, strlen(game)+1);
			if (ERROR_SUCCESS != res)
			{
				CheckError("Impossible d'enregistrer le nom du jeu dans les données de GameCtrl", res);
				result = FALSE;
			}
		}

		LONG res = RegSetValueEx(hTestKey, "SLOTS", 0, REG_BINARY, (BYTE*)&data.HourSlots[0], 24*sizeof(unsigned char));
		if (ERROR_SUCCESS != res)
		{
			CheckError("Impossible d'enregistrer les crénaux horaires", res);
			result = FALSE;
		}

		res = RegCloseKey(hTestKey);
	}
	else
		CheckError("Impossible de sauvegarder les données de GameCtrl", res);

	result = result && RevertToSelf();
	result = result && CloseHandle(token);

	return result;
}


