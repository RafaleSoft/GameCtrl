// DeviceInput.cpp: implementation of the CDeviceInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ISystem.h"
#include "DeviceInput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDeviceInput::CDeviceInput()
	:m_lpDirectInputDevice(NULL), m_lpDeviceInstance(NULL)
{

}

CDeviceInput::~CDeviceInput()
{
	if (m_lpDirectInputDevice != NULL)
		m_lpDirectInputDevice->Release();
}

bool CDeviceInput::CreateDevice(CISystem *ISystem, DWORD guid)
{
	if ((NULL == ISystem) || 
		((DI8DEVTYPE_KEYBOARD != guid) &&
		(DI8DEVTYPE_MOUSE != guid) &&
		(DI8DEVTYPE_GAMEPAD != guid) &&
		(DI8DEVTYPE_JOYSTICK != guid)))
	{
		return false;
	}

	m_lpDeviceInstance = ISystem->GetDInputInstance(guid);
	if (NULL == m_lpDeviceInstance)
		return false;
	
	HRESULT result = ISystem->getDirectInput()->CreateDevice(	m_lpDeviceInstance->guidInstance,
																&m_lpDirectInputDevice,
																NULL);
	if (result != DI_OK)
	{
		if (result == DIERR_DEVICENOTREG)
			MessageBox(NULL, "Device not registered !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_INVALIDPARAM)
			MessageBox(NULL, "Device not ready !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_NOINTERFACE)
			MessageBox(NULL, "Device has no available interface !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_NOTINITIALIZED)
			MessageBox(NULL, "Device not initialised !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_OUTOFMEMORY)
			MessageBox(NULL, "Device out of memory !", "Erreur", MB_OK | MB_ICONERROR);
		else
			MessageBox(NULL, "Unable to initialise Device due to unknown error !", "Erreur", MB_OK | MB_ICONERROR);
		
		return false;
	}
	else
	{
		result = m_lpDirectInputDevice->SetCooperativeLevel(ISystem->getHWND(), DISCL_BACKGROUND | DISCL_EXCLUSIVE);

		if (result == DIERR_INVALIDPARAM)
		{
			MessageBox(NULL, "Invalid parameter !", "Erreur", MB_OK | MB_ICONERROR);
			return false;
		}
		else if (result == DIERR_NOTINITIALIZED)
		{
			MessageBox(NULL, "Object not initialised !", "Erreur", MB_OK | MB_ICONERROR);
			return false;
		}

		m_capabilities.dwSize = sizeof(DIDEVCAPS);
		if (DI_OK != m_lpDirectInputDevice->GetCapabilities(&m_capabilities))
		{
			MessageBox(NULL, "Unable to read Device capabilities !", "Erreur", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return true;
}

const std::string CDeviceInput::GetName() const
{
	if (m_lpDeviceInstance != NULL)
		return std::string(m_lpDeviceInstance->tszInstanceName);
	else
		return "";
}

const std::string CDeviceInput::GetProductName() const
{
	if (m_lpDeviceInstance != NULL)
		return std::string(m_lpDeviceInstance->tszProductName);
	else
		return "";
}

const std::string CDeviceInput::GetTypeName() const
{
	std::string dname = "";
		
	if (m_lpDeviceInstance == NULL)
		return "";

	BYTE type = BYTE(m_lpDeviceInstance->dwDevType & 0xFF);
	BYTE subtype = BYTE((m_lpDeviceInstance->dwDevType>>8) & 0xFF);

	if (type == DI8DEVTYPE_DEVICE)
		dname +=" ??";

	if (m_lpDeviceInstance->dwDevType & DIDEVTYPE_HID)
		dname +=" HID";

	return dname;
}


bool CDeviceInput::SetEventNotification(HANDLE evt)
{
	if (NULL != m_lpDeviceInstance)
	{
		HRESULT res = m_lpDirectInputDevice->SetEventNotification(evt);
		bool set = ((res == DI_OK) || (res == DI_POLLEDDEVICE));
		return set;
	}
	else
		return false;
}


void* CDeviceInput::getDeviceState(size_t data_size, void* device_data)
{
	if (NULL == m_lpDirectInputDevice)
		return NULL;

	HRESULT result = m_lpDirectInputDevice->Acquire();
	if (result != DI_OK)
	{
		//MessageBox(NULL, "Device not acquired!", "Erreur", MB_OK | MB_ICONERROR);
		result = m_lpDirectInputDevice->Unacquire();
		result = m_lpDirectInputDevice->Acquire();
		if (result != DI_OK)
			return NULL;
	}

	if (pollingRequired())
	{
		result = m_lpDirectInputDevice->Poll();
		if ((DI_OK == result) || (DI_NOEFFECT == result))
		{
		}
		else if (DIERR_INPUTLOST == result)
		{
			MessageBox(NULL, "Device Input lost!", "Erreur", MB_OK | MB_ICONERROR);
			return NULL;
		}
		else if (DIERR_NOTACQUIRED == result)
		{
			MessageBox(NULL, "Device not acquired!", "Erreur", MB_OK | MB_ICONERROR);
			return NULL;
		}
		else if (DIERR_NOTINITIALIZED == result)
		{
			MessageBox(NULL, "Device not initialized!", "Erreur", MB_OK | MB_ICONERROR);
			return NULL;
		}
	}

	memset(device_data, 0, data_size);
	result = m_lpDirectInputDevice->GetDeviceState(data_size, device_data);

	if (result != DI_OK)
	{
		if (DIERR_INPUTLOST == result)
		{
			//MessageBox(NULL, "Device input lost!", "Erreur", MB_OK | MB_ICONERROR);
			return NULL;
		}
		else
		{
			MessageBox(NULL, "Device state unavailable!", "Erreur", MB_OK | MB_ICONERROR);
			return NULL;
		}
	}

	m_lpDirectInputDevice->Unacquire();
	return device_data;
}

bool CDeviceInput::registerAction(CAction* action)
{
	if (NULL == action)
		return false;

	for (size_t i = 0; i < m_actions.size(); i++)
	{
		if (m_actions[i] == action)
			return false;
	}

	m_actions.push_back(action);
	return true;
}
