// DeviceInput.cpp: implementation of the CDeviceInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <dinput.h>
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

std::string CDeviceInput::GetProductName() const
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

	if (type == DIDEVTYPE_MOUSE)
	{
		if (subtype == DIDEVTYPEMOUSE_UNKNOWN )
			dname +=" unknown";
		else if (subtype == DIDEVTYPEMOUSE_TRADITIONAL )
			dname +=" traditionnal";
		else if (subtype == DIDEVTYPEMOUSE_FINGERSTICK )
			dname +=" fingerstick";
		else if (subtype == DIDEVTYPEMOUSE_TOUCHPAD )
			dname +=" touchpad";
		else if (subtype == DIDEVTYPEMOUSE_TRACKBALL )
			dname +=" trackball";

		dname +=" mouse";
	}
	else if (type == DIDEVTYPE_KEYBOARD)
	{
		if (subtype == DIDEVTYPEKEYBOARD_UNKNOWN )
			dname +=" unknown";
		else if (subtype == DIDEVTYPEKEYBOARD_PCXT )
			dname +=" pc xt";
		else if (subtype == DIDEVTYPEKEYBOARD_OLIVETTI )
			dname +=" olivetti";
		else if (subtype == DIDEVTYPEKEYBOARD_PCAT )
			dname +=" pc at";
		else if (subtype == DIDEVTYPEKEYBOARD_PCENH )
			dname +=" pc enhanced";
		else if (subtype == DIDEVTYPEKEYBOARD_NOKIA1050 )
			dname +=" nokia 1050";
		else if (subtype == DIDEVTYPEKEYBOARD_NOKIA9140 )
			dname +=" nokia 9140";
		else if (subtype == DIDEVTYPEKEYBOARD_NEC98 )
			dname +=" nec 98";
		else if (subtype == DIDEVTYPEKEYBOARD_NEC98LAPTOP )
			dname +=" nec 98 laptop";
		else if (subtype == DIDEVTYPEKEYBOARD_NEC98106 )
			dname +=" nec 98106";
		else if (subtype == DIDEVTYPEKEYBOARD_JAPAN106 )
			dname +=" japan 106";
		else if (subtype == DIDEVTYPEKEYBOARD_JAPANAX )
			dname +=" japan ax";
		else if (subtype == DIDEVTYPEKEYBOARD_J3100 )
			dname +=" j3100";

		dname +=" keyboard";
	}
	else if (type == DIDEVTYPE_JOYSTICK)
	{
		if (subtype == DIDEVTYPEJOYSTICK_UNKNOWN )
			dname +="unknown";
		else if (subtype == DIDEVTYPEJOYSTICK_TRADITIONAL )
			dname +="traditional";
		else if (subtype == DIDEVTYPEJOYSTICK_FLIGHTSTICK )
			dname +="flistick";
		else if (subtype == DIDEVTYPEJOYSTICK_GAMEPAD )
			dname +="gamepad";
		else if (subtype == DIDEVTYPEJOYSTICK_RUDDER )
			dname +="rudder";
		else if (subtype == DIDEVTYPEJOYSTICK_WHEEL )
			dname +="wheel";
		else if (subtype == DIDEVTYPEJOYSTICK_HEADTRACKER )
			dname +="headtracker";

		dname +=" joystick";
	}
	else if (type == DIDEVTYPE_DEVICE)
		dname +=" ??";

	if (m_lpDeviceInstance->dwDevType & DIDEVTYPE_HID)
		dname +=" HID";

	return dname;
}


bool CDeviceInput::SetEventNotification(CEvent *evt)
{
	HRESULT res = m_lpDirectInputDevice->SetEventNotification(HANDLE(*evt));

	return ((res == DI_OK) || (DI_POLLEDDEVICE));
}


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