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

//	The one and only one main window.
extern HWND hWnd;

//	Utils interface
void	Error(DWORD msg);
void	CheckError(const char* msg, DWORD err);
BOOL	CheckInstall();
BOOL	runGame(const char *path);
BOOL	stopGame(void);
void	adjustGameTime(GameCtrlData_st &data);
BOOL	ParseCmdLine(LPSTR lpCmdLine);

//	ACL management
BOOL	IsUserAdmin(HANDLE token);
BOOL	SetSecurity(const char* file);
BOOL	FindUser(const char* USerName);
BOOL	CreateUser(const char* UserName);
BOOL	DeleteUser(const char* UserName);
BOOL	StartInteractiveClientProcess(LPTSTR lpCommandLine, HANDLE hToken);
BOOL	ExecuteAsAdmin(HWND hWnd, const char *file);

//	Registry helpers
BOOL	GetRegistryVars(GameCtrlData_st &data);
BOOL	SetRegistryVars(GameCtrlData_st &data);

// Dialog callbacks
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Config(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Games(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Password(HWND, UINT, WPARAM, LPARAM);
