// ISystem.cpp: implementation of the CISystem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <dinput.h>
#include "ISystem.h"


//////////////////////////////////////////////////////////////////////
// Direct Input callback
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumDevicesProc(LPCDIDEVICEINSTANCE lpddi,LPVOID pvRef)
{
	CArray<LPCDIDEVICEINSTANCE,LPCDIDEVICEINSTANCE>
					*devInstances = (CArray<LPCDIDEVICEINSTANCE,LPCDIDEVICEINSTANCE>*)pvRef;

	LPDIDEVICEINSTANCE lpcddi = new DIDEVICEINSTANCE;
	lpcddi->dwSize = lpddi->dwSize; 
    lpcddi->guidInstance = lpddi->guidInstance; 
    lpcddi->guidProduct = lpddi->guidProduct; 
    lpcddi->dwDevType = lpddi->dwDevType; 
    memcpy(lpcddi->tszInstanceName,lpddi->tszInstanceName,MAX_PATH); 
    memcpy(lpcddi->tszProductName,lpddi->tszProductName,MAX_PATH);
    lpcddi->guidFFDriver = lpddi->guidFFDriver;
    lpcddi->wUsagePage = lpddi->wUsagePage; 
    lpcddi->wUsage = lpddi->wUsage; 
	devInstances->Add(lpcddi);

	return DIENUM_CONTINUE;
}
 
UINT Poller( LPVOID pParam )
{
	CISystem::LPINPUTCOLLECTION coll = (CISystem::LPINPUTCOLLECTION)pParam;

	CEvent *lck = new CEvent(TRUE,TRUE,"ISystemPollerLock",NULL );
	CEvent *evt = new CEvent(FALSE,FALSE,"InputNotify",NULL );

	lck->SetEvent();
	evt->ResetEvent();
	CSingleLock	lock(lck,TRUE);

	HANDLE h = HANDLE(*evt);

	if (coll->k != NULL)
		coll->k->SetEventNotification(evt);
	if (coll->m != NULL)
		coll->m->SetEventNotification(evt);
	if (coll->c != NULL)
		coll->c->SetEventNotification(evt);

	while ( lock.IsLocked() )
	{
		DWORD res = 0;
		
		res = WaitForSingleObject(h,INFINITE);

		//	poll keyboard
		if (coll->k != NULL)
		{			
			LPCKEYBOARDSTATE ks = coll->k->getKeyboardState();
			if (ks != NULL)
			{
				DWORD key = coll->k->hasKeyboardData(0);

				while (key<256)
				{
					WORD K = ( ((WORD)(coll->k->getKeyboardData(key))) << 8) + ((WORD)(key));
					coll->k->m_buffer.Add(K);
					
					key = coll->k->hasKeyboardData(key);
				}
			}
		}

		//	poll mouse
		if (coll->m != NULL)
		{
		}

		//	poll controller
		if (coll->c != NULL)
		{
		}
	}

	delete coll;
	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CISystem::CISystem()
	:m_lpDirectInput(NULL)
{
	//m_closePollers = new CEvent(FALSE,FALSE,"ISystemPollerLock",NULL );
	//m_closePollers->ResetEvent();
}

CISystem::~CISystem()
{
	//m_closePollers->SetEvent();

	for (int i=0;i<m_pollers.size();i++)
	{
		WaitForSingleObject(m_pollers[i]->m_hThread,INFINITE);
	}
}

//////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////
bool CISystem::InitInputSystem(HINSTANCE hinst)
{
	if (DI_OK != DirectInput8Create(hinst, 
									DIRECTINPUT_VERSION, 
									IID_IDirectInput8,
									(LPVOID*)&m_lpDirectInput,
									NULL))
	{
		return false;
	}


	if (DI_OK != m_lpDirectInput->EnumDevices(	0, 
												&DIEnumDevicesProc, 
												&m_devInstances, 
												DIEDFL_ATTACHEDONLY))
	{
		return false;
	}

	return true;
}

void CISystem::CloseInputSystem()
{
	
	for (int i=0;i<m_devInstances.size();i++)
		delete ((LPDIDEVICEINSTANCE)(m_devInstances[i]));

	m_devInstances.clear();
}

LPCDIDEVICEINSTANCE CISystem::GetGUIDInstance(DWORD guid)
{
	bool found = false;
	int pos = 0;
	LPCDIDEVICEINSTANCE inst = NULL;

	while ((pos<m_devInstances.size())&&(!found))
	{
		LPCDIDEVICEINSTANCE	devInst = (LPCDIDEVICEINSTANCE)(m_devInstances[pos++]);
		if ( devInst->dwDevType & guid )
		{
			//	found only if first type equals
			if ((devInst->dwDevType&0x7)==(guid&0x7))
			{
				inst = devInst;
				found = true;
			}
		}
	}	
	return inst;
}

bool CISystem::startPoller( CKeyboardInput *keyb, CMouseInput *mouse, CControllerInput *ctrl)
{
	LPINPUTCOLLECTION coll = new INPUTCOLLECTION;
	coll->k = keyb;
	coll->m = mouse;
	coll->c = ctrl;

	CWinThread *poller = AfxBeginThread( Poller, coll, THREAD_PRIORITY_NORMAL, 0, 0, NULL );

	if (poller != NULL)
	{
		m_pollers.push_back(poller);
		return true;
	}
	else
		return false;
}

bool CISystem::stopPoller(void)
{
	if (0 != m_closePollers->SetEvent())
	{
		DWORD res = 0;

		for (int i=0;i<m_pollers.size();i++)
		{
			//	2 sec. should be enough for thread termination
			if (WAIT_OBJECT_0 != WaitForSingleObject(m_pollers[i]->m_hThread,2000))
				res = WAIT_FAILED; 
		}

		return (res == 0);
	}
	else
		return false;
}