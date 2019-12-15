// GamePad.cpp : Defines the utility functions for the application.
//

#include "stdafx.h"
#include "GameCtrl.h"
#include "ISystem.h"
#include "ControllerInput.h"


static CISystem *p_InputSystem = NULL;					// Controller manager
static const uint32_t NB_RETRY = 5;
static bool attached = false;

class MyButton : public CDeviceInput::CAction
{
public:
	MyButton() {};
	virtual ~MyButton() {};

	virtual bool execute(CDeviceInput::EVENTID event,
						 CDeviceInput::EVENTDATA data);
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
				DWORD key = 0x30;
				DWORD oem = 0;
				DWORD car = 0;
				switch (data)
				{
					case 0:
					{
						key = 0x31;
						oem = 0x00020001;
						car = 0x26;
						break;
					}
					case 1:
					{
						key = 0x32;
						oem = 0x00030001;
						car = 0xe9;
						break;
					}
					case 2:
					{
						key = 0x33;
						oem = 0x00040001;
						car = 0x22;
						break;
					}
					case 3:
					{
						key = 0x34;
						oem = 0x00050001;
						car = 0x27;
						break;
					}
					case 4:
					{
						key = 0x35;
						oem = 0x00060001;
						car = 0x28;
						break;
					}
					case 5:
					{
						key = 0x36;
						oem = 0x00070001;
						car = 0x2d;
						break;
					}
					case 6:
					{
						key = 0x37;
						oem = 0x00080001;
						car = 0xe8;
						break;
					}
					case 7:
					{
						key = 0x38;
						oem = 0x00090001;
						car = 0xf5;
						break;
					}
				}
								
				DWORD lTime = GetCurrentTime();
				res = SendMessage(gamechild, WM_KEYDOWN, key, lTime);
				res = SendMessage(gamechild, WM_KEYUP, key, lTime);
			}
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