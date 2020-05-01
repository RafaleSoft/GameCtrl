// ISystem.cpp: implementation of the CISystem class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ISystem.h"
#include "KeyboardInput.h"
#include "MouseInput.h"
#include "ControllerInput.h"

#include <wbemidl.h>
#include <oleauto.h>

#define SAFE_RELEASE(ppT) \
	if (ppT) \
	{ \
		ppT->Release(); \
		ppT = NULL; \
	}

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput)
{
	bool					bIsXinputDevice = false;
	IEnumWbemClassObject*	pEnumDevices = NULL;
	IWbemServices*			pIWbemServices = NULL;
	BSTR					bstrNamespace = NULL;
	BSTR					bstrClassName = NULL;
	BSTR					bstrDeviceID = NULL;

	// CoInit if needed
	HRESULT hr = CoInitialize(NULL);
	bool bCleanupCOM = SUCCEEDED(hr);

	// Create WMI
	IWbemLocator* pIWbemLocator = NULL;
	hr = CoCreateInstance(__uuidof(WbemLocator),
						  NULL,
						  CLSCTX_INPROC_SERVER,
						  __uuidof(IWbemLocator),
						  (LPVOID*)&pIWbemLocator);
	if (FAILED(hr) || pIWbemLocator == NULL)
		goto LCleanup;

	bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2"); if (bstrNamespace == NULL) goto LCleanup;
	bstrClassName = SysAllocString(L"Win32_PNPEntity");   if (bstrClassName == NULL) goto LCleanup;
	bstrDeviceID = SysAllocString(L"DeviceID");          if (bstrDeviceID == NULL)  goto LCleanup;

	// Connect to WMI 
	hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0L,
									  0L, NULL, NULL, &pIWbemServices);
	if (FAILED(hr) || pIWbemServices == NULL)
		goto LCleanup;

	// Switch security level to IMPERSONATE. 
	CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
					  RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

	hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, NULL, &pEnumDevices);
	if (FAILED(hr) || pEnumDevices == NULL)
		goto LCleanup;

	IWbemClassObject*	pDevices[20] = { 0 };
	// Loop over all devices
	for (;;)
	{
		DWORD				uReturned = 0;
		// Get 20 at a time
		hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned);
		if (FAILED(hr))
			goto LCleanup;
		if (uReturned == 0)
			break;

		for (UINT iDevice = 0; iDevice < uReturned; iDevice++)
		{
			// For each device, get its device ID
			VARIANT	var;
			hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, NULL, NULL);
			if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
			{
				// Check if the device ID contains "IG_".  If it does, then it's an XInput device
				// This information can not be found from DirectInput 
				if (wcsstr(var.bstrVal, L"IG_"))
				{
					// If it does, then get the VID/PID from var.bstrVal
					DWORD dwPid = 0, dwVid = 0;
					WCHAR* strVid = wcsstr(var.bstrVal, L"VID_");
					if (strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1)
						dwVid = 0;
					WCHAR* strPid = wcsstr(var.bstrVal, L"PID_");
					if (strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1)
						dwPid = 0;

					// Compare the VID/PID to the DInput device
					DWORD dwVidPid = MAKELONG(dwVid, dwPid);
					if (dwVidPid == pGuidProductFromDirectInput->Data1)
					{
						bIsXinputDevice = true;
						goto LCleanup;
					}
				}
			}
			SAFE_RELEASE(pDevices[iDevice]);
		}
	}

LCleanup:
	if (bstrNamespace)
		SysFreeString(bstrNamespace);
	if (bstrDeviceID)
		SysFreeString(bstrDeviceID);
	if (bstrClassName)
		SysFreeString(bstrClassName);
	for (UINT iDevice = 0; iDevice<20; iDevice++)
		SAFE_RELEASE(pDevices[iDevice]);
	SAFE_RELEASE(pEnumDevices);
	SAFE_RELEASE(pIWbemLocator);
	SAFE_RELEASE(pIWbemServices);

	if (bCleanupCOM)
		CoUninitialize();

	return bIsXinputDevice;
}


//////////////////////////////////////////////////////////////////////
// Direct Input callback
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumDevicesProc(LPCDIDEVICEINSTANCE lpddi,LPVOID pvRef)
{
	std::vector<LPCDIDEVICEINSTANCE> *devices = (std::vector<LPCDIDEVICEINSTANCE>*)pvRef;

	LPDIDEVICEINSTANCE lpcddi = new DIDEVICEINSTANCE;
	memcpy(lpcddi, lpddi, sizeof(DIDEVICEINSTANCE));
	
	devices->push_back(lpcddi);

	return DIENUM_CONTINUE;
}

//////////////////////////////////////////////////////////////////////
// Direct Input polling thread
//////////////////////////////////////////////////////////////////////
DWORD WINAPI Poller(LPVOID pParam)
{
	CISystem::LPINPUTCOLLECTION coll = (CISystem::LPINPUTCOLLECTION)pParam;

	HANDLE evt = CreateEvent(NULL, FALSE, FALSE, "InputNotify");
	bool res = false;
	if (coll->k != NULL)
		res = coll->k->SetEventNotification(evt);
	if (coll->m != NULL)
		res = coll->m->SetEventNotification(evt);
	if (coll->c != NULL)
		res = coll->c->SetEventNotification(evt);

	HANDLE closePollers = coll->pollerEvt;
	DWORD stop = WAIT_OBJECT_0;
	stop = WaitForSingleObject(closePollers, 1);

	while (WAIT_TIMEOUT == stop)
	{
		DWORD res = WaitForSingleObject(evt,5);
		if (WAIT_OBJECT_0 == res)
		{
			// getDeviceData
		}

		//	poll keyboard
		if (coll->k != NULL)
			coll->k->FillDeviceBuffer(true);

		//	poll mouse
		if (coll->m != NULL)
			coll->m->FillDeviceBuffer(true);

		//	poll controller
		if (coll->c != NULL)
			coll->c->FillDeviceBuffer(true);

		stop = WaitForSingleObject(closePollers, 0);
	}

	if (coll->k != NULL)
		coll->k->SetEventNotification(NULL);
	if (coll->m != NULL)
		coll->m->SetEventNotification(NULL);
	if (coll->c != NULL)
		coll->c->SetEventNotification(NULL);
	CloseHandle(evt);

	delete coll;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CISystem::CISystem()
	:m_lpDirectInput(NULL), m_closePollers(NULL)
{
}

CISystem::~CISystem()
{
	CloseInputSystem();

	if (NULL != m_closePollers)
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
												&m_devDInput,
												DIEDFL_ATTACHEDONLY))
	{
		return false;
	}

	LPCDIDEVICEINSTANCE	lpDeviceInstance = GetDInputInstance(DI8DEVTYPE_GAMEPAD);
	if (NULL != lpDeviceInstance)
	{
		if (IsXInputDevice(&lpDeviceInstance->guidProduct))
			m_devXInput.push_back(lpDeviceInstance);
	}
	lpDeviceInstance = GetDInputInstance(DI8DEVTYPE_JOYSTICK);
	if (NULL != lpDeviceInstance)
	{
		if (IsXInputDevice(&lpDeviceInstance->guidProduct))
			m_devXInput.push_back(lpDeviceInstance);
	}

	m_closePollers = CreateEvent(NULL,
								 FALSE, // BOOL bManualReset,
								 FALSE, // BOOL bInitialState,
								 "ISystemPollerLock");

	return true;
}

void CISystem::CloseInputSystem()
{
	stopPoller();

	for (size_t i = 0; i<m_devDInput.size(); i++)
		delete ((LPDIDEVICEINSTANCE)(m_devDInput[i]));

	m_devDInput.clear();
	m_devXInput.clear();

	m_lpDirectInput->Release();
	m_lpDirectInput = NULL;
}

LPCDIDEVICEINSTANCE CISystem::GetDInputInstance(DWORD guid) const
{
	bool found = false;
	size_t pos = 0;
	LPCDIDEVICEINSTANCE inst = NULL;

	while ((pos < m_devDInput.size()) && (!found))
	{
		LPCDIDEVICEINSTANCE	devInst = (LPCDIDEVICEINSTANCE)(m_devDInput[pos++]);
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

LPCDIDEVICEINSTANCE CISystem::GetXInputInstance(DWORD guid) const
{
	bool found = false;
	size_t pos = 0;
	LPCDIDEVICEINSTANCE inst = NULL;

	while ((pos < m_devXInput.size()) && (!found))
	{
		LPCDIDEVICEINSTANCE	devInst = (LPCDIDEVICEINSTANCE)(m_devXInput[pos++]);
		if (devInst->dwDevType & guid)
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
	coll->pollerEvt = m_closePollers;
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
	if (0 != SetEvent(m_closePollers))
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

