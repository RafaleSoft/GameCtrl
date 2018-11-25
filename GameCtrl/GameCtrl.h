#pragma once

#include "resource.h"


// Global data and registry structure
struct GameCtrlData_st
{
	int				CHRONO;					// Durée en minutes de jeu
	long			ReinitChrono;			// Valeur de réinitialisation du compteur
	long			NbDaysToReinit;			// Nb de jours avant réinitialisation
	FILETIME		NextUpdateTime;			// Prochaine réinitialisation des compteurs
	long			NbGames;				// Nombre de jeux installés
	const char**	Games;					// Chemin vers les lanceurs de jeux.
};

struct GameCtrlOptions_st
{
	BOOL doInstall;
	BOOL doUnInstall;
	BOOL doUsage;
	BOOL doVersion;
	BOOL doForce;
	BOOL doRun;
};

#define DEFAULT_BUFSIZE 256

//	The one and only one main window.
extern HWND hWnd;

//	Utils interface
wchar_t *toWchar(const char *text);
void	Error(DWORD msg);
void	Warning(DWORD msg);
void	Info(DWORD msg);
void	CheckError(const char* msg, DWORD err);
BOOL	CheckInstall(const GameCtrlData_st &data);
BOOL	runGame(const char *path);
BOOL	stopGame(void);
BOOL	adjustGameTime(GameCtrlData_st &data);
BOOL	adjustMenu(GameCtrlData_st &data);
BOOL	ParseCmdLine(LPSTR lpCmdLine, GameCtrlOptions_st &options);
BOOL	Install(BOOL force);
BOOL	UnInstall(BOOL force);

//	ACL management
BOOL	IsUserAdmin(HANDLE token);
BOOL	CheckSecurity(const char* file);
PACL	SetSecurity(PSECURITY_DESCRIPTOR psec);
PACL	UnsetSecurity(PSECURITY_DESCRIPTOR psec);
BOOL	FindUser(const char* USerName);
BOOL	CreateUser(const char* UserName);
BOOL	DeleteUser(const char* UserName);
PSECURITY_DESCRIPTOR GetFileDACL(const char* file);
BOOL	SetFileDACL(const char* file, PSECURITY_DESCRIPTOR psec, PACL pNewAcl);
BOOL	StartInteractiveClientProcess(LPTSTR lpCommandLine, HANDLE hToken);
BOOL	ExecuteAsAdmin(const char *file, const char *admin);
BOOL	AddAceToWindowStation(HWINSTA hwinsta, PSID psid);
BOOL	AddAceToDesktop(HDESK hdesk, PSID psid);
BOOL	ObtainSid(HANDLE hToken, PSID *psid);

//	Registry helpers
BOOL	CleanRegistry();
BOOL	InitRegistry(GameCtrlData_st &data);
BOOL	GetRegistryVars(GameCtrlData_st &data);
BOOL	SetRegistryVars(const GameCtrlData_st &data);

// Dialog callbacks
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Config(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Games(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Password(HWND, UINT, WPARAM, LPARAM);

// CryptoAPI helpers
BOOL initEncryption();
unsigned char* encrypt(const unsigned char* data, DWORD &dataSize);
unsigned char* decrypt(const unsigned char* data, DWORD &dataSize);
BOOL closeEncryption();