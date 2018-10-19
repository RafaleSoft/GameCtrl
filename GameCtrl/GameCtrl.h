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

struct GameCtrlOptions_st
{
	BOOL doInstall;
	BOOL doUnInstall;
	BOOL doUsage;
	BOOL doVersion;
};


//	The one and only one main window.
extern HWND hWnd;

//	Utils interface
void	Error(DWORD msg);
void	Warning(DWORD msg);
void	CheckError(const char* msg, DWORD err);
BOOL	CheckInstall(GameCtrlData_st &data);
BOOL	runGame(const char *path);
BOOL	stopGame(void);
void	adjustGameTime(GameCtrlData_st &data);
BOOL	ParseCmdLine(LPSTR lpCmdLine, GameCtrlOptions_st &options);
BOOL	Install(void);
BOOL	UnInstall(void);

//	ACL management
BOOL	IsUserAdmin(HANDLE token);
BOOL	SetSecurity(const char* file);
BOOL	FindUser(const char* USerName);
BOOL	CreateUser(const char* UserName);
BOOL	DeleteUser(const char* UserName);
BOOL	StartInteractiveClientProcess(LPTSTR lpCommandLine, HANDLE hToken);
BOOL	ExecuteAsAdmin(const char *file, const char *admin);

//	Registry helpers
BOOL	GetRegistryVars(GameCtrlData_st &data);
BOOL	SetRegistryVars(GameCtrlData_st &data);

// Dialog callbacks
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Config(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Games(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Password(HWND, UINT, WPARAM, LPARAM);
