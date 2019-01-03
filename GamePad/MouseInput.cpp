// MouseInput.cpp: implementation of the CMouseInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ISystem.h"
#include "MouseInput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMouseInput::CMouseInput(CISystem *ISystem)
{
	if (CreateDevice(ISystem, DI8DEVTYPE_MOUSE))
	{
		m_capabilities.dwSize = sizeof(DIDEVCAPS);
		if (DI_OK != m_lpDirectInputDevice->GetCapabilities(&m_capabilities))
			MessageBox(NULL, "Unable to read Device capabilities !", "Erreur", MB_OK | MB_ICONERROR);

		if (DI_OK != m_lpDirectInputDevice->SetDataFormat(&c_dfDIMouse))
			MessageBox(NULL, "Failed to set data format", "Erreur", MB_OK | MB_ICONERROR);
	}
}

CMouseInput::~CMouseInput()
{

}

const std::string CMouseInput::GetTypeName() const
{
	std::string dname = "";

	if (m_lpDeviceInstance == NULL)
		return "";

	BYTE type = BYTE(m_lpDeviceInstance->dwDevType & 0xFF);
	BYTE subtype = BYTE((m_lpDeviceInstance->dwDevType >> 8) & 0xFF);

	if (type == DI8DEVTYPE_MOUSE)
	{
		if (subtype == DI8DEVTYPEMOUSE_UNKNOWN)
			dname = "unknown";
		else if (subtype == DI8DEVTYPEMOUSE_TRADITIONAL)
			dname = "traditionnal";
		else if (subtype == DI8DEVTYPEMOUSE_FINGERSTICK)
			dname = "fingerstick";
		else if (subtype == DI8DEVTYPEMOUSE_TOUCHPAD)
			dname = "touchpad";
		else if (subtype == DI8DEVTYPEMOUSE_TRACKBALL)
			dname = "trackball";
		else if (subtype == DI8DEVTYPEMOUSE_ABSOLUTE)
			dname = "absolute";

		dname += " mouse";
		return dname;
	}
	else
		return CDeviceInput::GetTypeName();
}

LPCDIMOUSESTATE CMouseInput::getMouseState()
{
	HRESULT result;

	result = m_lpDirectInputDevice->Acquire();
	if (result != DI_OK)
	{
		MessageBox(NULL, "Device not acquired!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}

	result = m_lpDirectInputDevice->Poll();
	if (result != DI_OK)
	{
		MessageBox(NULL, "Device not polled!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}

	result = m_lpDirectInputDevice->GetDeviceState(sizeof(DIMOUSESTATE),&m_mouseState);
	m_lpDirectInputDevice->Unacquire();

	if (result != DI_OK)
	{
		MessageBox(NULL, "Device state unavailable!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}
	else
		return &m_mouseState;
}