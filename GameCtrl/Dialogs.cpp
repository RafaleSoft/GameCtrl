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

BOOL SetSecurity(const char* file)
{
	//SECURITY_INFORMATION RequestedInformation = ATTRIBUTE_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	//SECURITY_INFORMATION RequestedInformation = BACKUP_SECURITY_INFORMATION; == > ERROR_ACCESS_DENIED
	SECURITY_INFORMATION RequestedInformation = DACL_SECURITY_INFORMATION;
	PSECURITY_DESCRIPTOR pSecurityDescriptor;
	unsigned char buffer[256];
	DWORD              nLength = 256; // sizeof(SECURITY_DESCRIPTOR);
	DWORD              nLengthNeeded = 0;
	BOOL sec = GetFileSecurity(file, RequestedInformation, buffer /*&SecurityDescriptor*/, nLength, &nLengthNeeded);
	if (TRUE != sec)
	{
		DWORD err = GetLastError();
		MessageBox(NULL, file, "GetFileSecurity", MB_OK);
	}
	else
	{
		pSecurityDescriptor = (PSECURITY_DESCRIPTOR)buffer;

		SECURITY_DESCRIPTOR_CONTROL pControl;
		DWORD dwRevision = 0;
		GetSecurityDescriptorControl(pSecurityDescriptor, &pControl, &dwRevision);
		switch (pControl)
		{
			case SE_OWNER_DEFAULTED: break;	// (0x0001)
			case SE_GROUP_DEFAULTED: break; // (0x0002)
			case SE_DACL_PRESENT: break; // (0x0004)
			case SE_DACL_DEFAULTED: break; // (0x0008)
			case SE_SACL_PRESENT: break; // (0x0010)
			case SE_SACL_DEFAULTED: break; // (0x0020)
			case SE_DACL_AUTO_INHERIT_REQ: break; // (0x0100)
			case SE_SACL_AUTO_INHERIT_REQ: break; // (0x0200)
			case SE_DACL_AUTO_INHERITED: break; // (0x0400)
			case SE_SACL_AUTO_INHERITED: break; // (0x0800)
			case SE_DACL_PROTECTED: break; // (0x1000)
			case SE_SACL_PROTECTED: break; // (0x2000)
			case SE_RM_CONTROL_VALID: break; // (0x4000)
			case SE_SELF_RELATIVE: break; // (0x8000)
		}

		PACL dacl = NULL;
		BOOL DaclPresent = FALSE;
		BOOL DaclDefaulted = FALSE;
		GetSecurityDescriptorDacl(pSecurityDescriptor, &DaclPresent, &dacl, &DaclDefaulted);
		for (int j = 0; j < dacl->AceCount; j++)
		{
			LPVOID ace;
			sec = GetAce(dacl, j, &ace);
			if (TRUE != sec)
			{
				DWORD err = GetLastError();
				MessageBox(NULL, file, "GetFileSecurity", MB_OK);
			}
			else
			{
				ACE_HEADER *header = (ACE_HEADER*)ace;
				switch (header->AceFlags)
				{
					case OBJECT_INHERIT_ACE: break;
					case CONTAINER_INHERIT_ACE: break;
					case NO_PROPAGATE_INHERIT_ACE: break;
					case INHERIT_ONLY_ACE: break;
					case INHERITED_ACE: break;
					case SUCCESSFUL_ACCESS_ACE_FLAG: break;
					case FAILED_ACCESS_ACE_FLAG: break;

				}

				switch (header->AceType)
				{
					case ACCESS_ALLOWED_ACE_TYPE:
					{

						ACCESS_ALLOWED_ACE *allowed = (ACCESS_ALLOWED_ACE*)ace;
						DWORD sid = allowed->SidStart;
						DWORD access = allowed->Mask;
						break;
					}
					case ACCESS_ALLOWED_CALLBACK_ACE_TYPE:
					{
						break;
					}
					case ACCESS_ALLOWED_CALLBACK_OBJECT_ACE_TYPE:
					{
						break;
					}
					case ACCESS_ALLOWED_COMPOUND_ACE_TYPE:
					{
						break;
					}
					case ACCESS_ALLOWED_OBJECT_ACE_TYPE:
					{
						break;
					}
					case ACCESS_DENIED_ACE_TYPE:
					{
						break;
					}
				}
			}
		}

		//MessageBox(NULL, file, "GetFileSecurity", MB_OK);
	}

	return FALSE;
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

			HIMAGELIST imageList = ImageList_Create(32, 32, ILC_COLOR32, 16, 8);
			for (long i = 0; i < pSaveData->NbGames; i++)
			{
				HICON hh = ExtractIcon(0, pSaveData->Games[i], 0);
				ImageList_AddIcon(imageList, hh);
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

				SetSecurity(pSaveData->Games[i]);
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
				if ((INT_PTR)TRUE == DialogBox(NULL, MAKEINTRESOURCE(IDD_PASSWORD), hDlg, Password))
				{
					char buffer[256] = "\0\0";
					OPENFILENAME open;

					memset(&open, 0, sizeof(OPENFILENAME));
					open.lStructSize = sizeof(OPENFILENAME);
					open.hwndOwner = hDlg;
					open.lpstrFilter = "Game executable\0*.exe\0\0";
					open.nFilterIndex = 1;
					open.lpstrFile = buffer;
					open.nMaxFile = 256;

					if (GetOpenFileName(&open))
					{

					}
				}
				else
					MessageBox(hDlg, "Invalid username or password", "Error", MB_OK | MB_ICONERROR);
				break;
			}
			break;
		}
		default:
			break;
	}
	return (INT_PTR)FALSE;
}

