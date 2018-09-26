#pragma once

#include "resource.h"


// Global data and registry structure
struct GameCtrlData_st
{
	int				CHRONO;					// Durée en minutes de jeu
	long			ReinitChrono;
	long			NbDaysToReinit;			
	FILETIME		NextUpdateTime;			// Prochaine réinitialisation des compteurs
	long			NbGames;
	const char**	Games;
};


//	Utils interface
void	CheckError(HWND hWnd, const char* msg, DWORD err);
BOOL	runGame(HWND hWnd, const char *path);
BOOL	stopGame(HWND hWnd);
void	adjustGameTime(GameCtrlData_st &data);

//	Registry helpers
BOOL	GetRegistryVars(HWND hWnd, GameCtrlData_st &data);
BOOL	SetRegistryVars(HWND hWnd, GameCtrlData_st &data);

// Dialog callbacks
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Config(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Games(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Password(HWND, UINT, WPARAM, LPARAM);
