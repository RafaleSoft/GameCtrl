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
	static const uint32_t MAX_DATA = 64;

	typedef enum
	{
		DEVICE_KEYBOARD,
		DEVICE_MOUSE,
		DEVICE_CONTROLLER,
		DEVICE_OTHER
	} DEVICE_TYPE;

	typedef uint32_t EVENTID;
	typedef uint32_t EVENTDATA;

	class CAction
	{
	public:
		CAction() {};
		virtual ~CAction() {};
		virtual bool execute(EVENTID event, EVENTDATA data) = 0;
	};


public:
	CDeviceInput();
	virtual ~CDeviceInput();

	//!	
	virtual const std::string GetName() const;

	//!	
	virtual const std::string GetProductName() const;

	//! Returns the device type name (kind of a subtype)
	virtual const std::string GetTypeName() const;

	//! Returns the device type of the device
	virtual DEVICE_TYPE GetType() const = 0;

	//!	Register the device with the event to be notified when device state changes.
	bool SetEventNotification(HANDLE evt);

	//!	Fill the device buffer with immediate data
	virtual bool FillDeviceBuffer(bool doNotify = false) = 0;

	//!	Return true if this device requires polling to retrieve data.
	bool pollingRequired(void) const
	{
		return (DIDC_POLLEDDATAFORMAT == (m_capabilities.dwFlags & DIDC_POLLEDDATAFORMAT));
	}

	//!	Register a new action with this device.
	//! @return false if action could not registered or already exist.
	virtual bool registerAction(CAction* action);


protected:
	//!	Every device must call this creation method before any other action.
	bool CreateDevice(CISystem *ISystem, DWORD guid);

	//!	Obtain the device immediate data.
	void* getDeviceState(size_t data_size, void* device_data);

	//!	The device interface.
	LPDIRECTINPUTDEVICE8	m_lpDirectInputDevice;
	LPCDIDEVICEINSTANCE		m_lpDeviceInstance;
	DIDEVCAPS				m_capabilities;

	std::vector<CAction*>	m_actions;


private:
	DIDEVICEOBJECTDATA		m_data[MAX_DATA];

	unsigned int data;
};

#endif // !defined(AFX_DEVICEINPUT_H__4AF56246_51DA_4EAA_BF92_FBD647E6B2C7__INCLUDED_)
