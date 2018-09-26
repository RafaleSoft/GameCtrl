// GameCtrl.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hWnd = 0;


int CHRONO_DEFAULT = 140;
int DAYS_DEFAULT = 7;
UINT nIDEvent = 0;
HFONT font = 0;

GameCtrlData_st data = {	CHRONO_DEFAULT,
							CHRONO_DEFAULT,
							DAYS_DEFAULT,
							{ 0, 0 },
							1,
							NULL };


// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
void				adjustGameTime(GameCtrlData_st &data);


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

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_GAMECTRL, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GAMECTRL));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	KillTimer(hWnd, (UINT_PTR)&nIDEvent);

	SetRegistryVars(hWnd,data);

	// Wait until game process exits if any.
	stopGame(hWnd);

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
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
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, 200, 150, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		CheckError(hWnd, "Unable to create window", ::GetLastError());
		return FALSE;
	}

   
	if (!GetRegistryVars(hWnd,data))
		return FALSE;
	else
		adjustGameTime(data);

	if (0 == data.CHRONO)
	{
		::MessageBox(hWnd, "Temps de jeu dépassé !", "Erreur", MB_OK);
		return FALSE;
	}
	
	font = ::CreateFont(72, 0, 0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, TEXT("Times New Roman"));
	::ShowCursor(TRUE);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	if (0 == SetTimer(hWnd, (UINT_PTR)&nIDEvent, 1000 * 60, NULL))
	{
		CheckError(hWnd, "Error creating timer", ::GetLastError());
		return FALSE;
	}

	return TRUE;
}

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
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
		case WM_COMMAND:
		{
			wmId = LOWORD(wParam);
			wmEvent = HIWORD(wParam);
			// Parse the menu selections:
			switch (wmId)
			{
				case IDM_ABOUT:
					DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					break;
				case ID_CONFIG_TIMELIMITER:
					if ((INT_PTR)TRUE == DialogBox(hInst, MAKEINTRESOURCE(IDD_PASSWORD), hWnd, Password))
						DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_DIALOG_DELAYS), hWnd, Config, (LPARAM)&data);
					else
						MessageBox(hWnd, "Invalid username or password", "Error", MB_OK | MB_ICONERROR);
					break;
				case ID_CONFIG_GAMES:
					DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_GAMECTRL_DIALOG), hWnd, Games, (LPARAM)&data);
					break;
				case IDM_EXIT:
					// TODO : check game has been quit.
					DestroyWindow(hWnd);
					break;
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;
		}
		case WM_TIMER:
		{
			data.CHRONO = data.CHRONO - 1;
			//	Terminate current game if any when timer reaches 0
			if (0 == data.CHRONO)
				stopGame(hWnd);
			::InvalidateRect(hWnd, NULL, TRUE);
			break;
		}
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			::SelectObject(hdc, font);
			int hour = data.CHRONO / 60;
			int min = data.CHRONO - (hour * 60);
			char text[8];
			sprintf_s(text, "%02d:%02d", hour, min);
			::TextOut(hdc, 10, 10, text, 5);

			EndPaint(hWnd, &ps);
			break;
		}
		case WM_DESTROY:
		{
			//	Terminate current game if any,
			stopGame(hWnd);
			//	Quit application.
			PostQuitMessage(0);
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}



