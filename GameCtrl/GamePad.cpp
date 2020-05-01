// GamePad.cpp : Defines the utility functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include "ISystem.h"
#include "ControllerInput.h"
#include "MouseInput.h"


static CISystem *p_InputSystem = NULL;					// Controller manager
static const uint32_t NB_RETRY = 5;
static bool attached = false;

class MyButton : public CDeviceInput::CAction
{
public:
	MyButton()
	{
		k[0] = k[1] = 0;
		rstic_start = FALSE;
		dpad_start = FALSE;
		dpadx = dpady = lParam = 0;
	};

	virtual ~MyButton() {};

	virtual bool execute(CDeviceInput::EVENTID event,
						 CDeviceInput::EVENTDATA data);

private:
	DWORD	k[2];
	BOOL	rstic_start;
	BOOL	dpad_start;
	LONG	lParam;
	LONG	dpadx;
	LONG	dpady;
};


bool MyButton::execute(CDeviceInput::EVENTID event,
					   CDeviceInput::EVENTDATA data)
{
	LRESULT res = 0;

	switch (event)
	{
		case CControllerInput::BUTTON:
			if (NULL != hWnd)
			{
				DWORD key = 0;
				switch (data)
				{
					case 0: key = '1'; break;
					case 1: key = '2'; break;
					case 2: key = '3'; break;
					case 3: key = '4'; break;
					case 4: key = '5'; break;
					case 5: key = '6'; break;
					case 6: key = '7'; break;
					case 7: key = '8'; break;
				}

				if (key > 0)
				{
					DWORD lTime = GetCurrentTime();
					res = SendMessage(gamechild, WM_KEYDOWN, key, lTime);
					lTime = GetCurrentTime();
					res = SendMessage(gamechild, WM_KEYUP, key, lTime);
				}
			}
			break;
		case CControllerInput::LSTICK:
			if (NULL != hWnd)
			{
				DWORD lTime = GetCurrentTime();
				DWORD dx = (data & 0x0000FFFF);
				DWORD dy = (data & 0xFFFF0000) >> 16;

				// Center position may be offset by +/- 1 or zero.
				if (((dx == 0x7fff) || (dx == 0x7ffe) || (dx == 8000)) &&
					((dy == 0x7fff) || (dy == 0x7ffe) || (dy == 8000)))
				{
					if (0 != k[0])
						res = SendMessage(gamechild, WM_KEYUP, k[0], lTime);
					if (0 != k[1])
						res = SendMessage(gamechild, WM_KEYUP, k[1], lTime);
					k[1] = k[0] = 0;
				}
				else
				{
					LONG x = (LONG)dx - 32767;
					LONG y = (LONG)dy - 32767;

					DWORD numkeys = 0;
					DWORD keys[2] = { 0, 0 };
					DWORD keysup[2] = { 0, 0 };

					if (x > 10)							// Right direction
					{
						keys[numkeys] = 'D';
						keysup[numkeys++] = 'Q';
					}
					if (y > 10)							// Down direction
					{
						keys[numkeys] = 'S';
						keysup[numkeys++] = 'Z';
					}
					if (x < -10)							// Left direction
					{
						keys[numkeys] = 'Q';
						keysup[numkeys++] = 'D';
					}
					if (y < -10)							// Up direction
					{
						keys[numkeys] = 'Z';
						keysup[numkeys++] = 'S';
					}

					for (DWORD i=0; i < numkeys;i++)
					{
						DWORD lTime = GetCurrentTime();
						if ((0 != k[i]) && (keys[i] != k[i]))
							res = SendMessage(gamechild, WM_KEYUP, k[i], lTime);
						res = SendMessage(gamechild, WM_KEYDOWN, keys[i], lTime);
						k[i] = keys[i];
					}
				}
			}
			break;
		case CControllerInput::RSTICK:
			if (NULL != hWnd)
			{
				DWORD dx = (data & 0x0000FFFF);
				DWORD dy = (data & 0xFFFF0000) >> 16;

				LONG x = (LONG)dx - 32767;
				LONG y = (LONG)dy - 32767;

				if ((abs(x) < 10) && (abs(y) < 10))
				{
					if (TRUE == rstic_start)
					{
						rstic_start = FALSE;
						res = SendMessage(gamechild, WM_LBUTTONUP, 0, lParam);

						RECT o;
						GetWindowRect(gamechild, &o);
						RECT r;
						GetClientRect(gamechild, &r);

						LONG posx = 0.8 * (r.right - r.left);
						LONG posy = 0.725 * (r.bottom - r.top);
						SetCursorPos(o.left + posx, o.top + posy);
					}
				}
				else
				{
					RECT o;
					GetWindowRect(gamechild, &o);
					RECT r;
					GetClientRect(gamechild, &r);

					LONG posx = 0.8 * (r.right - r.left) + (x * 128) / 32768;
					LONG posy = 0.725 * (r.bottom - r.top) + (y * 128) / 32768;

					lParam = 0;
					lParam = ((posy & 0x0000FFFF) << 16) | (posx & 0x0000FFFF);

					DWORD lTime = GetCurrentTime();
					if (FALSE == rstic_start)
					{
						res = SendMessage(gamechild, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
						SetCursorPos(o.left + posx, o.top + posy);
					}
					else
					{
						//SetCursorPos(o.left + posx, o. top + posy);
						//res = SendMessage(gamechild, WM_MOUSEMOVE, MK_LBUTTON, lParam);
						
						DWORD mx = ((o.left + posx) * 65535) / 1920;
						DWORD my = ((o.top + posy) * 65535) / 1200;
						
						INPUT i;
						i.type = INPUT_MOUSE;
						i.mi.dx = mx;
						i.mi.dy = my;
						i.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
						i.mi.dwExtraInfo = 0;
						i.mi.mouseData = 0;
						i.mi.time = lTime;
						if (0 == SendInput(1, &i, sizeof(INPUT)))
							MessageBox(NULL, "SendInput blocked", "Error", MB_ICONERROR | MB_OKCANCEL);
					}

					rstic_start = TRUE;
				}
			}
			break;
		case CControllerInput::DPAD:
			if (NULL != hWnd)
			{
				if (data == (DWORD)-1)
				{
					if (dpad_start == TRUE)
					{
						dpad_start = FALSE;

						RECT o;
						GetWindowRect(gamechild, &o);
						RECT r;
						GetClientRect(gamechild, &r);

						LONG posx = (r.right - r.left) / 2;
						LONG posy = (r.bottom - r.top) / 2;

						DWORD dparam = (posx & 0x0000ffff) | ((posy & 0x0000ffff) << 16);
						res = SendMessage(gamechild, WM_LBUTTONUP, 0, dparam);
						SetCursorPos(o.left + posx, o.top + posy);
					}
				}
				else
				{
					RECT o;
					GetWindowRect(gamechild, &o);
					RECT r;
					GetClientRect(gamechild, &r);

					if (FALSE == dpad_start)
					{
						LONG posx = (r.right - r.left) / 2;
						LONG posy = (r.bottom - r.top) / 2;

						DWORD dparam = (posx & 0x0000ffff) | ((posy & 0x0000ffff) << 16);

						res = SendMessage(gamechild, WM_LBUTTONDOWN, 0, dparam);
						SetCursorPos(o.left + posx, o.top + posy);
					}
					else
					{
						DWORD angle = data / 100;
						if ((angle > (45 / 2)) && (angle < (180 - 45 / 2)))
							dpadx += 1;
						if ((angle > (180 + 45 / 2)) && (angle < (360 - 45 / 2)))
							dpadx -= 1;
						if ((angle > (270 + 45 / 2)) || (angle < (90 - 45 / 2)))
							dpady -= 1;
						if ((angle > (90 + 45 / 2)) && (angle < (270 - 45 / 2)))
							dpady += 1;

						LONG posx = dpadx + (r.right - r.left) / 2;
						LONG posy = dpady + (r.bottom - r.top) / 2;

						DWORD dparam = (posx & 0x0000ffff) | ((posy & 0x0000ffff) << 16);
						
						res = SendMessage(gamechild, WM_MOUSEMOVE, 0, dparam);
						SetCursorPos(o.left + posx, o.top + posy);
					}

					dpad_start = TRUE;
				}
			}
		default:
			break;
	}

	return (res == TRUE);
}

class MyMouse : public CDeviceInput::CAction
{
public:
	MyMouse() {};
	virtual ~MyMouse() {};

	virtual bool execute(CDeviceInput::EVENTID event,
						 CDeviceInput::EVENTDATA data);
};

bool MyMouse::execute(CDeviceInput::EVENTID event, CDeviceInput::EVENTDATA data)
{
	LRESULT res = 0;

	switch (event)
	{
		case CMouseInput::BUTTON:
			res = TRUE;
			break;
		case CMouseInput::MOVE:
			res = TRUE;
			break;
		default:
			break;
	}

	return (res == TRUE);
}

BOOL attachGamePad(HINSTANCE hInst)
{
	if (NULL == hInst)
		return false;

	if (attached)
		return TRUE;

	if (NULL != p_InputSystem)
	{
		delete p_InputSystem;
		p_InputSystem = NULL;
	}
	
	GetWindowGame();

	if ((NULL != gameWnd) && (NULL != gamechild))
	{
		p_InputSystem = new CISystem();
		if (p_InputSystem->InitInputSystem(hInst, gamechild))
		{
			CControllerInput *controller = new CControllerInput(p_InputSystem);
			controller->registerAction(new MyButton());

			//CMouseInput *mouse = new CMouseInput(p_InputSystem);
			//mouse->registerAction(new MyMouse());

			p_InputSystem->startPoller(NULL, NULL, controller);
			attached = true;
		}
		else
		{
			Error(IDS_GAMEWND);
			return FALSE;
		}
	}
	else
		return FALSE;

	return TRUE;
}

BOOL detachGamePad(void)
{ 
	attached = false;
	if (NULL == p_InputSystem)
		return FALSE;

	bool res = p_InputSystem->stopPoller();
	delete p_InputSystem;
	p_InputSystem = NULL;

	return (res);
}