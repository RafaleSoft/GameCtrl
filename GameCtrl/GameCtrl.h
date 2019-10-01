#pragma once

#include "resource.h"


extern const char *USER_NAME;
extern const char *PASSWORD;

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
	BOOL doReset;
};

#define DEFAULT_BUFSIZE 256

//	The one and only one main window.
extern HWND hWnd;
//	The main game window.
extern HWND gameWnd;
// The top level game windows
extern HWND gameWnds[16];
// The number of top level game windows
extern size_t nbWnds;


//	Utils interface
wchar_t *toWchar(const char *text);
void	Error(DWORD msg);
void	Warning(DWORD msg);
void	Info(DWORD msg);
void	CheckError(const char* msg, DWORD err);
BOOL	CheckInstall(GameCtrlData_st &data);
BOOL	runGame(const char *path);
HWND	GetWindowGame(void);
BOOL	stopGame(void);
BOOL	ParseCmdLine(LPSTR lpCmdLine, GameCtrlOptions_st &options);
BOOL	Install(BOOL force, GameCtrlData_st &data);
BOOL	UnInstall(BOOL force);
BOOL	Reset(void);

//	ACL management
BOOL	IsUserAdmin(HANDLE token);
BOOL	CheckSecurity(const char* file);
PACL	SetSecurity(PSECURITY_DESCRIPTOR psec);
PACL	UnsetSecurity(PSECURITY_DESCRIPTOR psec);
BOOL	FindUser(const char* USerName);
BOOL	CreateUser(const char* UserName, const char* PassWord);
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

//	GamePad helpers
BOOL attachGamePad(HINSTANCE hInst);
BOOL detachGamePad(void);