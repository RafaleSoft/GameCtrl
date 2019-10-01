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
		if (DI_OK != m_lpDirectInputDevice->SetDataFormat(&c_dfDIMouse))
			MessageBox(NULL, "Failed to set data format", "Erreur", MB_OK | MB_ICONERROR);

		memset(&m_mouseState, 0, sizeof(DIMOUSESTATE));
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

bool CMouseInput::FillDeviceBuffer(bool doNotify)
{
	if (NULL == m_lpDirectInputDevice)
		return false;

	LPCDIMOUSESTATE data = (LPCDIMOUSESTATE)getDeviceState(sizeof(DIMOUSESTATE), &m_mouseState);
	if (data != NULL)
	{
		if (doNotify)
		{
			for (size_t i = 0; i < m_actions.size(); i++)
				m_actions[i]->execute(0,0);
		}
		return true;
	}
	else
		return false;
}

