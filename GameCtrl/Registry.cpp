// Regsitry.cpp : Defines registry functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>


BOOL CleanRegistry()
{
	HKEY hTestKey = 0;
	REGSAM keySAM = DELETE | KEY_SET_VALUE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE;
	LONG res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"), 0, keySAM, &hTestKey);

	if (ERROR_SUCCESS == res)
	{
		res = RegDeleteTree(hTestKey, NULL);
		if (ERROR_SUCCESS == res)
		{
			RegCloseKey(hTestKey);
			res = RegDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"));
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

	LONG res = RegCreateKeyEx(HKEY_CURRENT_USER,
							  TEXT("Software\\GameCtrl"), 0, NULL,
							  REG_OPTION_NON_VOLATILE,
							  KEY_WRITE,
							  NULL,
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
		return FALSE;
	}

	return TRUE;
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

	res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"), 0, KEY_READ, &hTestKey);
	
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
				CheckError("Unable to read game name", res);
				RegCloseKey(hTestKey);
				return FALSE;
			}
		}

		RegCloseKey(hTestKey);
		return TRUE;
	}
	else
	{
		CheckError("Unable to get GameCtrl data", res);
		return FALSE;
	}
}



BOOL SetRegistryVars(const GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	LONG res = 0;

	res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"), 0, KEY_SET_VALUE, &hTestKey);
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
		
		for (long i = 0; i < data.NbGames; i++)
		{
			const char* game = data.Games[i];
			char buffer[32];
			sprintf_s(buffer, 32, "GAME%d", i);
			res = RegSetValueEx(hTestKey, TEXT(buffer), 0, REG_SZ, (BYTE*)game, strlen(game)+1);
			if (ERROR_SUCCESS != res)
			{
				CheckError("Unable to write game name", res);
				RegCloseKey(hTestKey);
				return FALSE;
			}
		}

		RegCloseKey(hTestKey);
		return TRUE;
	}
	else
	{
		CheckError("Unable to write GameCtrl data", res);
		return FALSE;
	}
}


