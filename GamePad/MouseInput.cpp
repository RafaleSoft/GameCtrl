// MouseInput.cpp: implementation of the CMouseInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <dinput.h>
#include "ISystem.h"
#include "MouseInput.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMouseInput::CMouseInput(CISystem *ISystem)
{
	m_lpDeviceInstance = ISystem->GetGUIDInstance(DI8DEVTYPE_MOUSE);
	LPDIRECTINPUTDEVICE lp;

	HRESULT result = ISystem->m_lpDirectInput->CreateDevice(m_lpDeviceInstance->guidInstance, 
															&lp,
															NULL);
	if (result != DI_OK)
	{
		if (result == DIERR_DEVICENOTREG )
			MessageBox(NULL, "Device not registered !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_INVALIDPARAM )
			AfxMessageBox("Device not resdy !");
		else if (result == DIERR_NOINTERFACE  )
			AfxMessageBox("Device has no available interface !");
		else if (result == DIERR_NOTINITIALIZED  )
			AfxMessageBox("Device not initialised !");
		else if (result == DIERR_OUTOFMEMORY   )
			AfxMessageBox("Device out of memory !");
		else
			AfxMessageBox("Unable to initialise Device due to unknown error !");
	}
 
	lp->QueryInterface(IID_IDirectInputDevice2,(void **)&m_lpDirectInputDevice);
	lp->Release();

	result = m_lpDirectInputDevice->SetCooperativeLevel(ISystem->m_hWnd,DISCL_BACKGROUND|DISCL_NONEXCLUSIVE);

	if (result == DIERR_INVALIDPARAM)
		AfxMessageBox("Invalid parameter !");
	else if (result == DIERR_NOTINITIALIZED )
		AfxMessageBox("Object not initialised !");

	m_capabilities.dwSize = sizeof(DIDEVCAPS);
	if (DI_OK != m_lpDirectInputDevice->GetCapabilities(&m_capabilities))
		AfxMessageBox("Unable to read Device capabilities !");

	if (DI_OK != m_lpDirectInputDevice->SetDataFormat(&c_dfDIMouse))
		AfxMessageBox("Failed to set data format");
}

CMouseInput::~CMouseInput()
{

}

LPCDIMOUSESTATE CMouseInput::getMouseState()
{
	HRESULT result;

	result = m_lpDirectInputDevice->Acquire();
	if (result != DI_OK)
	{
		AfxMessageBox("Device not acquired!");
		return NULL;
	}

	result = m_lpDirectInputDevice->Poll();
	if (result != DI_OK)
	{
		AfxMessageBox("Device not polled!");
		return NULL;
	}

	result = m_lpDirectInputDevice->GetDeviceState(sizeof(DIMOUSESTATE),&m_mouseState);
	m_lpDirectInputDevice->Unacquire();

	if (result != DI_OK)
	{
		AfxMessageBox("Device state unavailable!");
		return NULL;
	}
	else
		return &m_mouseState;
}