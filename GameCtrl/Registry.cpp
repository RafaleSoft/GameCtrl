// Regsitry.cpp : Defines registry functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>



BOOL GetRegistryVars(HWND hWnd, GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	LONG res = 0;

	res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"), 0, KEY_READ, &hTestKey);
	
	if (res == ERROR_SUCCESS)
	{
		DWORD pdwType = 0;
		DWORD pvData = 0;
		DWORD pcbData = sizeof(pvData);

		res = RegGetValue(hTestKey, NULL, TEXT("CHRONO"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to read Chrono value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.CHRONO = pvData;

		res = RegGetValue(hTestKey, NULL, TEXT("REINITCHRONO"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to read Chrono reinit value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.ReinitChrono = pvData;
		
		res = RegGetValue(hTestKey, NULL, TEXT("NBDAYSTOREINIT"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if ((ERROR_SUCCESS != res) || (0 == pvData))
		{
			CheckError(hWnd, "Unable to read Nb days to reinit value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.NbDaysToReinit = pvData;

		res = RegGetValue(hTestKey, NULL, TEXT("LOWDATETIME"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to read LowFileTime value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.NextUpdateTime.dwLowDateTime = pvData;

		res = RegGetValue(hTestKey, NULL, TEXT("HIGHDATETIME"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to read HighFileTime value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.NextUpdateTime.dwHighDateTime = pvData;

		res = RegGetValue(hTestKey, NULL, TEXT("NBGAMES"), RRF_RT_REG_DWORD, &pdwType, &pvData, &pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to read Nb games value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		data.NbGames = pvData;
		if (NULL != data.Games)
		{
			for (long i = 0; i < data.NbGames; i++)
				delete[] data.Games[i];
			delete[] data.Games;
		}

		data.Games = new const char*[data.NbGames];

		for (long i = 0; i < data.NbGames; i++)
		{
			const char* game = data.Games[i];
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
				CheckError(hWnd, "Unable to read game name", res);
				RegCloseKey(hTestKey);
				return FALSE;
			}
		}

		RegCloseKey(hTestKey);
		return TRUE;
	}
	else
	{
		CheckError(hWnd, "Unable to get GameCtrl data", res);
		return FALSE;
	}
}

BOOL SetRegistryVars(HWND hWnd, GameCtrlData_st &data)
{
	HKEY hTestKey = 0;
	LONG res = 0;

	res = RegOpenKeyEx(HKEY_CURRENT_USER, TEXT("Software\\GameCtrl"), 0, KEY_SET_VALUE, &hTestKey);
	if (res == ERROR_SUCCESS)
	{
		DWORD pvData = data.CHRONO;
		DWORD pcbData = sizeof(pvData);

		res = RegSetValueEx(hTestKey, TEXT("CHRONO"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write Chrono value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		pvData = data.ReinitChrono;
		res = RegSetValueEx(hTestKey, TEXT("REINITCHRONO"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write Chrono reinit value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		pvData = data.NbDaysToReinit;
		res = RegSetValueEx(hTestKey, TEXT("NBDAYSTOREINIT"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write Nb days to reinit value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		pvData = data.NextUpdateTime.dwLowDateTime;
		res = RegSetValueEx(hTestKey, TEXT("LOWDATETIME"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write NbJours value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		pvData = data.NextUpdateTime.dwHighDateTime;
		res = RegSetValueEx(hTestKey, TEXT("HIGHDATETIME"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write NbJours value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		pvData = data.NbGames;
		res = RegSetValueEx(hTestKey, TEXT("NBGAMES"), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
		if (ERROR_SUCCESS != res)
		{
			CheckError(hWnd, "Unable to write Nb games value", res);
			RegCloseKey(hTestKey);
			return FALSE;
		}

		for (long i = 0; i < data.NbGames; i++)
		{
			const char* game = data.Games[i];
			char buffer[32];
			sprintf_s(buffer, 32, "GAME%d", i);
			res = RegSetValueEx(hTestKey, TEXT(buffer), 0, REG_SZ, (BYTE*)game, strlen(game)+1);
			if (ERROR_SUCCESS != res)
			{
				CheckError(hWnd, "Unable to write game name", res);
				RegCloseKey(hTestKey);
				return FALSE;
			}
		}

		RegCloseKey(hTestKey);
		return TRUE;
	}
	else
	{
		CheckError(hWnd, "Unable to write GameCtrl data", res);
		return FALSE;
	}
}


BOOL WriteDWORD(HWND hWnd, HKEY hKey, const char* name, DWORD pvData)
{
	if ((0 == hWnd) || (0 == hKey))
		return FALSE;

	DWORD pcbData = sizeof(pvData);
	LONG res = RegSetValueEx(hKey, TEXT(name), 0, REG_DWORD, (BYTE*)&pvData, pcbData);
	if (ERROR_SUCCESS != res)
	{
		CheckError(hWnd, "Unable to write dword value", res);
		return FALSE;
	}

	return TRUE;
}

