// ISystem.cpp: implementation of the CISystem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ISystem.h"
#include "KeyboardInput.h"


//////////////////////////////////////////////////////////////////////
// Direct Input callback
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumDevicesProc(LPCDIDEVICEINSTANCE lpddi,LPVOID pvRef)
{
	std::vector<LPCDIDEVICEINSTANCE> *devInstances = (std::vector<LPCDIDEVICEINSTANCE>*)pvRef;

	LPDIDEVICEINSTANCE lpcddi = new DIDEVICEINSTANCE;
	memcpy(lpcddi, lpddi, sizeof(DIDEVICEINSTANCE));

	/*
	lpcddi->dwSize = lpddi->dwSize; 
    lpcddi->guidInstance = lpddi->guidInstance; 
    lpcddi->guidProduct = lpddi->guidProduct; 
    lpcddi->dwDevType = lpddi->dwDevType; 
    memcpy(lpcddi->tszInstanceName,lpddi->tszInstanceName,MAX_PATH); 
    memcpy(lpcddi->tszProductName,lpddi->tszProductName,MAX_PATH);
    lpcddi->guidFFDriver = lpddi->guidFFDriver;
    lpcddi->wUsagePage = lpddi->wUsagePage; 
    lpcddi->wUsage = lpddi->wUsage;
	*/
	
	devInstances->push_back(lpcddi);

	return DIENUM_CONTINUE;
}
 
DWORD WINAPI Poller(LPVOID pParam)
{
	CISystem::LPINPUTCOLLECTION coll = (CISystem::LPINPUTCOLLECTION)pParam;

	/*
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
	*/

	DWORD stop = WAIT_OBJECT_0;
	//stop = WaitForSingleObject(m_closePollers, 0));

	while ( WAIT_OBJECT_0 == stop )
	{
		DWORD res = 0;
		
		//res = WaitForSingleObject(h,INFINITE);

		//	poll keyboard
		CKeyboardInput *k = coll->k;
		if (k != NULL)
		{			
			LPCKEYBOARDSTATE ks = k->getKeyboardState();
			if (ks != NULL)
			{
				DWORD key = k->hasKeyboardData(0);

				while (key<256)
				{
					WORD K = ( ((WORD)(k->getKeyboardData(key))) << 8) + ((WORD)(key));
					//k->m_buffer.push_back(K);
					
					key = k->hasKeyboardData(key);
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

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CISystem::CISystem()
	:m_lpDirectInput(NULL)
{
	m_closePollers = CreateEvent(	NULL, 
									FALSE, // BOOL bManualReset,
									TRUE, // BOOL bInitialState,
									"ISystemPollerLock");
}

CISystem::~CISystem()
{
	CloseInputSystem();

	CloseHandle(m_closePollers);
}

//////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////
bool CISystem::InitInputSystem(HINSTANCE hinst, HWND hWnd)
{
	if ((NULL == hinst) || (NULL == hWnd))
		return false;

	m_hWnd = hWnd;

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
	stopPoller();

	for (size_t i=0;i<m_devInstances.size();i++)
		delete ((LPDIDEVICEINSTANCE)(m_devInstances[i]));

	m_devInstances.clear();
	m_lpDirectInput->Release();

	m_lpDirectInput = NULL;
}

LPCDIDEVICEINSTANCE CISystem::GetGUIDInstance(DWORD guid) const
{
	bool found = false;
	size_t pos = 0;
	LPCDIDEVICEINSTANCE inst = NULL;

	while ((pos < m_devInstances.size()) && (!found))
	{
		LPCDIDEVICEINSTANCE	devInst = (LPCDIDEVICEINSTANCE)(m_devInstances[pos++]);
		if ( devInst->dwDevType & guid )
		{
			//	found only if first type equals
			if ((devInst->dwDevType & 0x7) == (guid & 0x7))
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

	HANDLE poller = NULL;
	DWORD ThreadId = 0;
	poller = CreateThread(NULL, 0, Poller, coll, CREATE_SUSPENDED, &ThreadId);

	if (poller != NULL)
	{
		m_pollers.push_back(poller);
		return ((DWORD)-1 != ResumeThread(poller));
	}
	else
		return false;
}

bool CISystem::stopPoller(void)
{
	if (0 != ResetEvent(m_closePollers))
	{
		DWORD res = 0;

		for (size_t i=0; i<m_pollers.size(); i++)
		{
			if (WAIT_OBJECT_0 != WaitForSingleObject(m_pollers[i], INFINITE))
			{
				res = WAIT_FAILED;
				return false;
			}
			else
				CloseHandle(m_pollers[i]);
		}
		if (0 == res)
			m_pollers.clear();
		return (res == 0);
	}
	else
		return false;
}