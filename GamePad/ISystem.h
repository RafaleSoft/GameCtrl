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
	 * @param h: the main window which will send/receive input
	 * @return true if initialization is correct and complete.
	 */
	bool InitInputSystem(HINSTANCE hinst);

	/*
	 * Release all the resources used by the input system.
	 */
	void CloseInputSystem();


	bool startPoller( CKeyboardInput *keyb, CMouseInput *mouse = NULL, CControllerInput *ctrl = NULL);
	bool stopPoller(void);


private:

	friend class CControllerInput;
	friend class CKeyboardInput;
	friend class CMouseInput;

	LPDIRECTINPUT8	m_lpDirectInput;
	std::vector<LPCDIDEVICEINSTANCE>	m_devInstances;
	LPCDIDEVICEINSTANCE GetGUIDInstance(DWORD guid);

	//CEvent						*m_closePollers;
	std::vector<CWinThread*>	m_pollers;
};

#endif // !defined(AFX_ISYSTEM_H__05BE7126_25DF_42ED_9B77_E91CB12F528B__INCLUDED_)
