// DeviceInput.h: interface for the CDeviceInput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICEINPUT_H__4AF56246_51DA_4EAA_BF92_FBD647E6B2C7__INCLUDED_)
#define AFX_DEVICEINPUT_H__4AF56246_51DA_4EAA_BF92_FBD647E6B2C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GamePad.h"

class CISystem;


class GAMEPAD_API CDeviceInput
{
public:
	static const unsigned int MAX_DATA = 64;

	typedef enum
	{
		DEVICE_KEYBOARD,
		DEVICE_MOUSE,
		DEVICE_CONTROLLER,
		DEVICE_OTHER
	} DEVICE_TYPE;

public:
	CDeviceInput();
	virtual ~CDeviceInput();

	virtual const std::string GetName() const;
	virtual const std::string GetProductName() const;
	virtual const std::string GetTypeName() const;

	virtual DEVICE_TYPE GetType() const = 0;

	//bool SetEventNotification(CEvent *evt);
	bool GetDeviceState(void);

	unsigned int data;

protected:
	bool CreateDevice(CISystem *ISystem, DWORD guid);

	LPDIRECTINPUTDEVICE2	m_lpDirectInputDevice;
	LPCDIDEVICEINSTANCE		m_lpDeviceInstance;
	DIDEVCAPS				m_capabilities;


private:
	DIDEVICEOBJECTDATA		m_data[MAX_DATA];
};

#endif // !defined(AFX_DEVICEINPUT_H__4AF56246_51DA_4EAA_BF92_FBD647E6B2C7__INCLUDED_)
