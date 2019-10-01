// ISystem.h: interface for the CISystem class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISYSTEM_H__05BE7126_25DF_42ED_9B77_E91CB12F528B__INCLUDED_)
#define AFX_ISYSTEM_H__05BE7126_25DF_42ED_9B77_E91CB12F528B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GamePad.h"


class CKeyboardInput;
class CMouseInput;
class CControllerInput;
class CWinThread;



class GAMEPAD_API CISystem
{
public:
	typedef struct INPUTCOLLECTION_t
	{
		HANDLE				pollerEvt;
		CKeyboardInput		*k;
		CMouseInput			*m;
		CControllerInput	*c;
	} INPUTCOLLECTION;
	typedef INPUTCOLLECTION	*LPINPUTCOLLECTION;


public:
	CISystem();
	virtual ~CISystem();

	/*
	 * Initialize the input system layer.
	 * @param hinst: the module instance which will manage input
	 * @param hwnd: the window that will attach the device input
	 * @return true if initialization is correct and complete.
	 */
	bool InitInputSystem(HINSTANCE hinst, HWND hWnd);

	/*
	 * Release all the resources used by the input system.
	 */
	void CloseInputSystem();

	/*
	 * Run a dedicated thread for device polling and execute actions.
	 */
	bool startPoller( CKeyboardInput *keyb = NULL, CMouseInput *mouse = NULL, CControllerInput *ctrl = NULL);
	
	/*
	 * Stop the dedicated thread started here above.
	 * Resources are not freed, only a thread is terminated.
	 */
	bool stopPoller(void);

	/*
	 * Returns the Direct Input Legacy device instance object for the given guid.
	 */
	LPCDIDEVICEINSTANCE GetDInputInstance(DWORD guid) const;

	/*
	* Returns the Direct Input Legacy device instance object for the given guid.
	*/
	LPCDIDEVICEINSTANCE GetXInputInstance(DWORD guid) const;
	
	/*
	 * Returns the direct input interface.
	 */
	LPDIRECTINPUT8	getDirectInput(void) const
	{ return m_lpDirectInput; }

	/*
	 * Returns the attached window.
	 */
	HWND getHWND(void) const
	{ return m_hWnd; }



private:

	LPDIRECTINPUT8	m_lpDirectInput;
	std::vector<LPCDIDEVICEINSTANCE>	m_devDInput;
	std::vector<LPCDIDEVICEINSTANCE>	m_devXInput;
	
	HWND m_hWnd;
	HANDLE m_closePollers;
	std::vector<HANDLE>	m_pollers;
};

#endif // !defined(AFX_ISYSTEM_H__05BE7126_25DF_42ED_9B77_E91CB12F528B__INCLUDED_)
