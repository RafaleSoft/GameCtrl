// Dialogs.cpp : Defines dialog callbacks for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include <stdio.h>


static DWORD LOGON_MODEL = LOGON32_LOGON_NETWORK;
static GameCtrlData_st *pSaveData = NULL;
static int selectedGame = -1;


// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
		case WM_INITDIALOG:
			return (INT_PTR)TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for authentication box.
INT_PTR CALLBACK Password(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR res = (INT_PTR)FALSE;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			SetDlgItemText(hDlg, IDC_EDIT_USERNAME, "");
			SetDlgItemText(hDlg, IDC_EDIT_PASSWORD, "");
			res = (INT_PTR)TRUE;

			DWORD param = (DWORD)lParam;
			LOGON_MODEL = param;
			if (0 == LOGON_MODEL)
				LOGON_MODEL = LOGON32_LOGON_NETWORK;
			
			break;
		}
		case WM_COMMAND:
		{
			if (LOWORD(wParam) == IDOK) 
			{
				char user[32] = "";
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT_USERNAME), user, 32);
				char pass[32] = "";
				GetWindowText(GetDlgItem(hDlg, IDC_EDIT_PASSWORD), pass, 32);

				HANDLE token = NULL;
				BOOL logon = LogonUser(user, ".", pass, LOGON_MODEL, LOGON32_PROVIDER_DEFAULT, &token);
				if (TRUE == logon)
				{
					if (((LOGON32_LOGON_NETWORK == LOGON_MODEL) && IsUserAdmin(token)) ||
						(LOGON32_LOGON_INTERACTIVE == LOGON_MODEL))
					{
						EndDialog(hDlg, LOWORD(wParam));
						res = (INT_PTR)TRUE;
					}
					else
						EndDialog(hDlg, IDCANCEL);
				}
				else
					EndDialog(hDlg, IDCANCEL);
			}
			else if (LOWORD(wParam) == IDCANCEL)
				EndDialog(hDlg, IDCANCEL);
			break;
		}
	}

	return res;
}


// Message handler for config dialog.
INT_PTR CALLBACK Config(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR res = (INT_PTR)FALSE;

	switch (message)
	{
		//case WM_SETFONT:
		//case WM_WINDOWPOSCHANGING:
		//case WM_NCACTIVATE:
		//case WM_GETICON:
		//case WM_ACTIVATE:
		//case WM_USER:
		//case WM_CHANGEUISTATE:
		//case WM_DWMNCRENDERINGCHANGED:
		//case WM_ACTIVATEAPP:
		//case WM_SHOWWINDOW:
		//case WM_NCPAINT:
		//case WM_ERASEBKGND:
		//case WM_CTLCOLORDLG:
		//case WM_CTLCOLORSTATIC:
		//case WM_CTLCOLOREDIT:
		//case WM_CTLCOLORBTN:
		//	res = (INT_PTR)FALSE;
		case WM_INITDIALOG:
		{
			pSaveData = (GameCtrlData_st*)lParam;
			if (NULL != pSaveData)
			{
				int radio = IDC_RADIO_DAY;
				if (1 == pSaveData->NbDaysToReinit)
					radio = IDC_RADIO_DAY;
				else if (7 == pSaveData->NbDaysToReinit)
					radio = IDC_RADIO_WEEK;
				else
					radio = IDC_RADIO_MONTH;

				CheckRadioButton(hDlg, IDC_RADIO_DAY, IDC_RADIO_MONTH, radio);

				char buffer[32] = "";
				sprintf_s(&buffer[0], 32, "%d", pSaveData->ReinitChrono);
				SetDlgItemText(hDlg, IDC_EDIT_MINUTES, buffer);
				res = (INT_PTR)TRUE;
			}
			break;
		}

		case WM_COMMAND:
		{
			if ((LOWORD(wParam) == IDOK) && (NULL != pSaveData))
			{
				if (IsDlgButtonChecked(hDlg, IDC_RADIO_DAY))
					pSaveData->NbDaysToReinit = 1;
				else if (IsDlgButtonChecked(hDlg, IDC_RADIO_WEEK))
					pSaveData->NbDaysToReinit = 7;
				else if (IsDlgButtonChecked(hDlg, IDC_RADIO_MONTH))
					pSaveData->NbDaysToReinit = 30;

				char buffer[32] = "";
				GetWindowText(GetDlgItem(hDlg,IDC_EDIT_MINUTES), buffer, 32);
				pSaveData->ReinitChrono = atoi(buffer);

				EndDialog(hDlg, LOWORD(wParam));
				res = (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				res = (INT_PTR)TRUE;
			}
			break;
		}
		default:
			break;
	}

	return res;
}


// Message handler for games dialog.
INT_PTR CALLBACK Games(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	INT_PTR res = (INT_PTR)FALSE;

	switch (message)
	{
		case WM_INITDIALOG:
		{
			pSaveData = (GameCtrlData_st*)lParam;

			HIMAGELIST imageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 16, 8);
			for (long i = 0; i < pSaveData->NbGames; i++)
			{
				HICON hh = ExtractIcon(0, pSaveData->Games[i], 0);
				ImageList_AddIcon(imageList, hh);
				DestroyIcon(hh);
			}

			HWND lv = GetDlgItem(hDlg, IDC_LIST_GAMES);
			ListView_SetImageList(lv, imageList, LVSIL_SMALL);
			
			for (long i = 0; i < pSaveData->NbGames; i++)
			{
				LVITEM item;
				memset(&item, 0, sizeof(LVITEM));
				item.iItem = i;
				item.mask = LVIF_TEXT | LVIF_IMAGE;
				item.iImage = i;
				const char *exe = strrchr(pSaveData->Games[i],'\\');
				item.pszText = (LPSTR)(exe+1);
				ListView_InsertItem(lv, &item);
			}

			return (INT_PTR)TRUE;
			break;
		}
		case WM_NOTIFY:
		{
			switch (LOWORD(wParam)) // hit control
			{
				case IDC_LIST_GAMES:
				{
					if (((LPNMHDR)lParam)->code == NM_CLICK)
					{
						HWND lv = GetDlgItem(hDlg, IDC_LIST_GAMES);
						selectedGame = ListView_GetNextItem(lv, -1, LVNI_SELECTED);
						HWND run = GetDlgItem(hDlg, IDOK);
						EnableWindow(run, (selectedGame != -1));
						//MessageBox(NULL, pSaveData->Games[iSelect], "Item selected", MB_OK);
					}
					break;
				}
			}
			break;
		}
		case WM_COMMAND:
		{
			//	User clicks back, return normaly.
			if (LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			//	User clicks run, run game and close window.
			else if (LOWORD(wParam) == IDOK)
			{
				runGame(pSaveData->Games[selectedGame]);
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDADD)
			{
				if ((INT_PTR)TRUE == DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_PASSWORD), hDlg, Password, LOGON32_LOGON_NETWORK))
				{
					char buffer[DEFAULT_BUFSIZE] = "\0\0";
					OPENFILENAME open;

					memset(&open, 0, sizeof(OPENFILENAME));
					open.lStructSize = sizeof(OPENFILENAME);
					open.hwndOwner = hDlg;
					open.lpstrFilter = "Game executable\0*.exe\0\0";
					open.nFilterIndex = 1;
					open.lpstrFile = buffer;
					open.nMaxFile = DEFAULT_BUFSIZE;

					
					// TODO: move to IFileOpenDialog
					if (FALSE == GetOpenFileName(&open))
						return (INT_PTR)FALSE;

					for (long i = 0; i < pSaveData->NbGames; i++)
					{
						if (!strcmp(pSaveData->Games[i], buffer))
						{
							Error(IDS_GAMEPREEXISTS);
							return (INT_PTR)FALSE;
						}
					}

					size_t buflen = strlen(buffer);
					if (buflen > 0)
					{
						PSECURITY_DESCRIPTOR psec = GetFileDACL(buffer);
						if (NULL != psec)
						{
							PACL newDacl = SetSecurity(psec);
							if (NULL != newDacl)
							{
								if (FALSE == SetFileDACL(buffer, psec, newDacl))
									Error(IDS_GAMEUNHANDLED);
								else
								{
									const char **new_Games = new const char*[pSaveData->NbGames + 1];

									for (long i = 0; i < pSaveData->NbGames; i++)
										new_Games[i] = pSaveData->Games[i];

									new_Games[pSaveData->NbGames] = new char[buflen + 1];
									memcpy((char*)new_Games[pSaveData->NbGames], buffer, buflen + 1);

									pSaveData->NbGames = pSaveData->NbGames + 1;
									delete[] pSaveData->Games;
									pSaveData->Games = new_Games;
								}
							}
							delete psec;
						}
					}
					return (INT_PTR)TRUE;
				}
				else
					Error(IDS_INVALIDUSER);
				break;
			}
			break;
		}
		default:
			break;
	}
	return (INT_PTR)FALSE;
}

