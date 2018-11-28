// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>
#include <CommCtrl.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE	hInst;								// current instance
HWND		hWnd = NULL;

static int SECONDS = 0;
static const int CHRONO_DEFAULT = 140;
static const int DAYS_DEFAULT = 7;
UINT nIDEvent = 0;
HFONT font = 0;
HPEN pen_white = 0;
HBRUSH	brush_white = 0;
HBRUSH	brush_red = 0;
HBRUSH current_brush = 0;

GameCtrlData_st data = {	CHRONO_DEFAULT,
							CHRONO_DEFAULT,
							DAYS_DEFAULT,
							{ 0, 0 },
							0,
							NULL };
GameCtrlOptions_st options = { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE };



//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:
		{
			int wmId = LOWORD(wParam);
			int wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
				{
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				}
				case ID_CONFIG_TIMELIMITER:
				{
					if ((INT_PTR)TRUE == DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_PASSWORD), hWnd, Password, LOGON32_LOGON_NETWORK))
						DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_DELAYS), hWnd, Config, (LPARAM)&data);
					else
						Error(IDS_INVALIDUSER);
					break;
				}
				case ID_CONFIG_GAMES:
				{
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_GAMECTRL_DIALOG), hWnd, Games, (LPARAM)&data);
					break;
				}
				case ID_CONFIG_UNINSTALL:
				{
					HINSTANCE hInstance = GetModuleHandle(NULL);
					TCHAR		szMessage[DEFAULT_BUFSIZE];
					LoadString(hInstance, IDS_CONFIRMREMOVE, szMessage, DEFAULT_BUFSIZE);
					
					int yesno = MessageBox(hWnd, szMessage, "Désinstallation GameCtrl", MB_ICONASTERISK | MB_YESNO);
					if (IDYES == yesno)
					{
						CleanRegistry();

						for (long i = 0; i < data.NbGames; i++)
						{
							PSECURITY_DESCRIPTOR psec = GetFileDACL(data.Games[i]);
							if (NULL != psec)
							{
								PACL newDacl = UnsetSecurity(psec);
								if (NULL != newDacl)
								{
									if (FALSE == SetFileDACL(data.Games[i], psec, newDacl))
										Error(IDS_GAMEUNHANDLED);
								}
							}
						}

						char buffer[MAX_LOADSTRING];
						GetModuleFileName(NULL, buffer, MAX_LOADSTRING);
						ExecuteAsAdmin(buffer, "--uninstall");

						exit(-1);
					}

					break;
				}
				case IDM_EXIT:
				{
					// TODO : check game has been quit.
					DestroyWindow(hWnd);
					break;
				}
				case IDM_GAME1:
				case IDM_GAME2:
				case IDM_GAME3:
				case IDM_GAME4:
				case IDM_GAME5:
				case IDM_GAME6:
				case IDM_GAME7:
				case IDM_GAME8:
				case IDM_GAME9:
				case IDM_GAME10:
				case IDM_GAME11:
				case IDM_GAME12:
				case IDM_GAME13:
				case IDM_GAME14:
				case IDM_GAME15:
				case IDM_GAME16:
				{
					runGame(data.Games[wmId - IDM_GAME1]);
					break;
				}
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		}
		case WM_TIMER:
		{
			// blink color for the last minute of gaming.
			if (1 == data.CHRONO)
			{
				if (brush_red == current_brush)
					current_brush = brush_white;
				else
					current_brush = brush_red;
			}

			SECONDS = SECONDS + 1;
			if (SECONDS > 59)
			{
				SECONDS = 0;
				data.CHRONO = data.CHRONO - 1;
			}
			//	Terminate current game if any when timer reaches 0
			if (0 == data.CHRONO)
				stopGame();
			InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case WM_ERASEBKGND:
		{
			HDC hdc = (HDC)wParam;
			SelectObject(hdc, pen_white);
			SelectObject(hdc, current_brush);

			RECT rect;
			GetClientRect(hWnd, &rect);
			Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

			break;
		}
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			
			SelectObject(hdc, font);

			int hour = data.CHRONO / 60;
			int min = data.CHRONO - (hour * 60);
			char text[8];
			sprintf_s(text, "%02d:%02d", hour, min);
			
			if (current_brush == brush_red)
				SetTextColor(hdc, RGB(255, 255, 255));
			else
				SetTextColor(hdc, RGB(0, 0, 0));
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 10, 10, text, 5);

			EndPaint(hWnd, &ps);
			break;
		}
		case WM_DESTROY:
		{
			//	Terminate current game if any,
			stopGame();
			//	Quit application.
			PostQuitMessage(0);
			break;
		}
		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
			break;
		}
	}
	return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance, TCHAR *szWindowClass)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GAMECTRL));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_GAMECTRL);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	TCHAR		szTitle[MAX_LOADSTRING];			// The title bar text
	TCHAR		szWindowClass[MAX_LOADSTRING];		// the main window class name

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GAMECTRL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance, szWindowClass);

	hInst = hInstance; // Store instance handle in our global variable
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, 200, 150, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		CheckError("Impossible de créer la fenêtre principale de l'application:", ::GetLastError());
		return FALSE;
	}
	else
	{
		HICON admin = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AUTHENTICATION));
		ICONINFO ii;
		GetIconInfo(admin, &ii);
		HBITMAP hBitmapUnchecked = ii.hbmColor;
		HBITMAP hBitmapChecked = ii.hbmColor;

		HMENU menu = GetMenu(hWnd);
		if (NULL == menu)
			return FALSE;

		HMENU config = GetSubMenu(menu, 1);
		if (NULL == config)
			return FALSE;

		if (FALSE == SetMenuItemBitmaps(config, 0, MF_BYPOSITION, hBitmapUnchecked, hBitmapChecked))
			return FALSE;

		HICON help = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HELP));
		GetIconInfo(help, &ii);
		hBitmapUnchecked = ii.hbmColor;
		hBitmapChecked = ii.hbmColor;

		menu = GetMenu(hWnd);
		if (NULL == menu)
			return FALSE;

		HMENU about = GetSubMenu(menu, 2);
		if (NULL == about)
			return FALSE;

		if (FALSE == SetMenuItemBitmaps(about, 0, MF_BYPOSITION, hBitmapUnchecked, hBitmapChecked))
			return FALSE;
	}


	//DWORD dataSize = 16;
	//initEncryption();
	//unsigned char* cypher = encrypt((unsigned char*)"Test chiffrement", dataSize);
	//unsigned char* text = decrypt(cypher, dataSize);
	//closeEncryption();

	//!	Collect application data from registry and compute next deadline.
	//!	If game is not properly installed, do the necessary actions below.
	if (FALSE == CheckInstall(data))
	{
		TCHAR		szMessage[DEFAULT_BUFSIZE];
		LoadString(hInstance, IDS_NOTINSTALLED, szMessage, DEFAULT_BUFSIZE);
		
		int yesno = MessageBox(hWnd, szMessage, "Installation de GameCtrl", MB_ICONASTERISK | MB_YESNO);
		if (IDYES == yesno)
		{
			char buffer[MAX_LOADSTRING];
			GetModuleFileName(NULL, buffer, MAX_LOADSTRING);
			if (TRUE == InitRegistry(data))
				ExecuteAsAdmin(buffer, "--install");
		}
		else
			return FALSE;
	}

	
	if (0 == data.CHRONO)
	{
		Error(IDS_OUTOFTIME);
		return FALSE;
	}
	
	pen_white = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	brush_white = CreateSolidBrush(RGB(255, 255, 255));
	brush_red = CreateSolidBrush(RGB(255, 0, 0));
	current_brush = brush_white;

	font = CreateFont(72, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Times New Roman"));
	ShowCursor(TRUE);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (0 == SetTimer(hWnd, (UINT_PTR)&nIDEvent, 1000, NULL))
	{
		CheckError("Error creating timer", ::GetLastError());
		return FALSE;
	}

	return TRUE;
}



//
//	Main entry point.
//
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	INITCOMMONCONTROLSEX init;
	init.dwSize = sizeof(INITCOMMONCONTROLSEX);
	init.dwICC = ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_LINK_CLASS;
	InitCommonControlsEx(&init);
	
	if (FALSE == ParseCmdLine(lpCmdLine, options))
	{
		Error(IDS_USAGE);
		return -1;
	}
	else
	{
		BOOL normalLaunch = FALSE;
		BOOL res = TRUE;
		if (options.doInstall)
			res = Install(options.doForce);
		else if (options.doUnInstall)
			res = UnInstall(options.doForce);
		else if (options.doVersion)
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
		else if (options.doUsage)
			Info(IDS_USAGE);
		else if (options.doReset)
			Reset();
		else	// No options : normal launch
			normalLaunch = TRUE;

		if (FALSE == normalLaunch)
		{
			if (FALSE == res)
				return -1;
			else
				return 1;
		}
	}
	
	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECTRL));

	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillTimer(hWnd, (UINT_PTR)&nIDEvent);

	//!	Save application data to registry.
	SetRegistryVars(data);

	// Wait until game process exits if any.
	stopGame();

	DeleteObject(pen_white);
	DeleteObject(brush_white);
	DeleteObject(brush_red);
	DeleteObject(font);

	return (int)msg.wParam;
}


