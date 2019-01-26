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
	:m_lpDirectInputDevice(NULL),
	m_lpDeviceInstance(NULL)
{

}

CDeviceInput::~CDeviceInput()
{
	if (m_lpDirectInputDevice != NULL)
		m_lpDirectInputDevice->Release();
}

const std::string CDeviceInput::GetName() const
{
	if (m_lpDeviceInstance != NULL)
		return std::string(m_lpDeviceInstance->tszInstanceName);
	else
		return "";
}

bool CDeviceInput::CreateDevice(CISystem *ISystem, DWORD guid)
{
	if ((NULL == ISystem) || 
		((DI8DEVTYPE_KEYBOARD != guid) &&
		(DI8DEVTYPE_MOUSE != guid) &&
		(DI8DEVTYPE_GAMEPAD != guid)))
	{
		return false;
	}

	m_lpDeviceInstance = ISystem->GetGUIDInstance(guid);
	
	LPDIRECTINPUTDEVICE8 lp = NULL;
	HRESULT result = ISystem->getDirectInput()->CreateDevice(m_lpDeviceInstance->guidInstance,
															&lp,
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
		lp->QueryInterface(IID_IDirectInputDevice2, (void **)&m_lpDirectInputDevice);
		lp->Release();

		result = m_lpDirectInputDevice->SetCooperativeLevel(ISystem->getHWND(), 
															DISCL_FOREGROUND | DISCL_EXCLUSIVE);

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

/*
bool CDeviceInput::SetEventNotification(CEvent *evt)
{
	HRESULT res = m_lpDirectInputDevice->SetEventNotification(HANDLE(*evt));

	return ((res == DI_OK) || (DI_POLLEDDEVICE));
}
*/

bool CDeviceInput::GetDeviceState(void)
{
	DWORD	nbData = MAX_DATA;

	HRESULT res = 
		m_lpDirectInputDevice->GetDeviceData(	sizeof(DIDEVICEOBJECTDATA),          
												m_data,	&nbData, 0);

	if (SUCCEEDED(res))
	{
		if (nbData>0)
		{
			data = m_data[0].dwData;
			return true;
		}
		else
			return false;
	}
	else
		return false;
}