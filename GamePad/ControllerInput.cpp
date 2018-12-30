// ControllerInput.cpp: implementation of the CControllerInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <dinput.h>
#include "ISystem.h"
#include "ControllerInput.h"

//////////////////////////////////////////////////////////////////////
// Callbacks
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumEffectsProc(LPCDIEFFECTINFO pdei,LPVOID pvRef)
{
	std::vector<LPCDIEFFECTINFO> *effectInstances = (std::vector<LPCDIEFFECTINFO>*)pvRef;

	LPDIEFFECTINFO pcdei = new DIEFFECTINFO;
	pcdei->dwSize = pdei->dwSize; 
    pcdei->guid = pdei->guid; 
    pcdei->dwEffType = pdei->dwEffType; 
    pcdei->dwStaticParams = pdei->dwStaticParams; 
    memcpy(pcdei->tszName,pdei->tszName,MAX_PATH); 
    pcdei->dwDynamicParams = pdei->dwDynamicParams;
    
	effectInstances->push_back(pcdei);

	return DIENUM_CONTINUE;
}
 
BOOL CALLBACK DIEnumDeviceObjectsProc(LPCDIDEVICEOBJECTINSTANCE lpddoi,PVOID pvRef)
{
	std::vector<LPDIDEVICEOBJECTINSTANCE> *objects = (std::vector<LPDIDEVICEOBJECTINSTANCE>*)pvRef;

	LPDIDEVICEOBJECTINSTANCE pobj = new DIDEVICEOBJECTINSTANCE;

	*pobj = *lpddoi;

	objects->push_back(pobj);

	return DIENUM_CONTINUE;
}
 
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CControllerInput::CControllerInput(CISystem *ISystem)
{
	for (int i=0;i<NB_FF_EFFECTS;i++)
		m_effects[i]=NULL;

	m_lpDeviceInstance = ISystem->GetGUIDInstance(DI8DEVTYPE_GAMEPAD);
	LPDIRECTINPUTDEVICE lp;

	HRESULT result = ISystem->m_lpDirectInput->CreateDevice(m_lpDeviceInstance->guidInstance, 
															&lp,
															NULL);
	if (result != DI_OK)
	{
		if (result == DIERR_DEVICENOTREG )
			MessageBox(NULL, "Device not registered !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_INVALIDPARAM )
			MessageBox(NULL, "Device not ready !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_NOINTERFACE  )
			MessageBox(NULL, "Device has no available interface !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_NOTINITIALIZED  )
			MessageBox(NULL, "Device not initialised !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_OUTOFMEMORY   )
			MessageBox(NULL, "Device out of memory !", "Erreur", MB_OK | MB_ICONERROR);
		else
			MessageBox(NULL, "Unable to initialise Device due to unknown error !", "Erreur", MB_OK | MB_ICONERROR);
	}
	else
	{
		lp->QueryInterface(IID_IDirectInputDevice2,(void **)&m_lpDirectInputDevice);
		lp->Release();

		result = m_lpDirectInputDevice->SetCooperativeLevel(ISystem->m_hWnd,DISCL_FOREGROUND|DISCL_EXCLUSIVE);

		if (result == DIERR_INVALIDPARAM)
			MessageBox(NULL, "Invalid parameter !", "Erreur", MB_OK | MB_ICONERROR);
		else if (result == DIERR_NOTINITIALIZED )
			MessageBox(NULL, "Object not initialised !", "Erreur", MB_OK | MB_ICONERROR);

		m_capabilities.dwSize = sizeof(DIDEVCAPS);
		if (DI_OK != m_lpDirectInputDevice->GetCapabilities(&m_capabilities))
			MessageBox(NULL, "Unable to read Device capabilities !", "Erreur", MB_OK | MB_ICONERROR);

		if (m_capabilities.dwFlags & DIDC_FORCEFEEDBACK)
			m_lpDirectInputDevice->EnumEffects(DIEnumEffectsProc,&m_effectInstances,DIEFT_ALL );

		DIPROPDWORD property;
		property.diph.dwSize = sizeof(DIPROPDWORD);
		property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		property.diph.dwHow = DIPH_DEVICE;
		property.diph.dwObj = 0;
		property.dwData = DIPROPAUTOCENTER_OFF;

		if (DI_OK != m_lpDirectInputDevice->SetProperty(DIPROP_AUTOCENTER,&(property.diph)))
			MessageBox(NULL, "Unable to set device property", "Erreur", MB_OK | MB_ICONERROR);

		if (DI_OK != m_lpDirectInputDevice->SetDataFormat(&c_dfDIJoystick2))
			MessageBox(NULL, "Failed to set data format", "Erreur", MB_OK | MB_ICONERROR);

		if (DI_OK != m_lpDirectInputDevice->EnumObjects(DIEnumDeviceObjectsProc,&m_objects,DIDFT_ALL))
			MessageBox(NULL, "Failed to read controller objects", "Erreur", MB_OK | MB_ICONERROR);
		else
		{
			m_cstForce.axes = new DWORD[1];
			m_cstForce.direction = new LONG[1];
			m_periodicForce.axes = new DWORD[1];
			m_periodicForce.direction = new LONG[1];
		}
	}
}

CControllerInput::~CControllerInput()
{	
	for (int i=0;i<m_effectInstances.size();i++)
		delete ((LPDIEFFECTINFO)(m_effectInstances[i]));

	m_effectInstances.clear();

	for (unsigned int i=0;i<NB_FF_EFFECTS;i++)
		if (m_effects[i] != NULL)
			m_effects[i]->Release();
}




LPCDIJOYSTATE2 CControllerInput::getControllerState()
{
	HRESULT result;

	if (m_lpDirectInputDevice == NULL)
		return &m_controllerState;

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

	memset(&m_controllerState,0,sizeof(DIJOYSTATE2));
	result = m_lpDirectInputDevice->GetDeviceState(sizeof(DIJOYSTATE2),&m_controllerState);
	m_lpDirectInputDevice->Unacquire();

	if (result != DI_OK)
	{
		MessageBox(NULL, "Device state unavailable!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}
	else
		return &m_controllerState;
}

bool CControllerInput::LoadEffects(EffectType effectType,bool unload)
{
	GUID guid;
	LPDIRECTINPUTEFFECT *pdeff = NULL;

	switch(effectType)
	{
		case CONSTANTFORCE:
			guid = GUID_ConstantForce;
			break;
		case RAMPFORCE:
			guid = GUID_RampForce;
			break;
		case SQUARE:
			guid = GUID_Square;
			break;
		case SINE:
			guid = GUID_Sine;
			break;
		case TRIANGLE:
			guid = GUID_Triangle;
			break;
		case SAWTOOTHUP:
			guid = GUID_SawtoothUp;
			break;
		case SAWTOOTHDOWN:
			guid = GUID_SawtoothDown;
			break;
		case SPRING:
			guid = GUID_Spring;
			break;
		case DAMPER:
			guid = GUID_Damper;
			break;
		case INERTIA:
			guid = GUID_Inertia;
			break;
		case FRICTION:
			guid = GUID_Friction;
			break;
		case CUSTOMFORCE:
			guid = GUID_CustomForce ;
			break;
	}

	int pos = 0;
	bool found = false;
	int max = m_effectInstances.size();

	while ((!found)&&(pos<NB_FF_EFFECTS)&&(pos<max))
	{
		if (m_effectInstances[pos]->guid==guid)
			found = true;
		else
			pos++;
	}

	if (found)
		pdeff = &(m_effects[pos]);
	else
		return false;

	if (*pdeff!=NULL)
	{
		(*pdeff)->Release();
		delete *pdeff;
	}

	if (!unload)
	{
		HRESULT res;
		res = m_lpDirectInputDevice->Acquire();
		if (res != DI_OK)
		{
			MessageBox(NULL, "Device not acquired!", "Erreur", MB_OK | MB_ICONERROR);
			return false;
		}	

		LPDIEFFECT effect = new DIEFFECT;
			
		InitEffect(effect,effectType);

		m_effectsSettings[pos] = effect;

		bool result = (DI_OK == m_lpDirectInputDevice->CreateEffect(guid,effect,pdeff,NULL));
		
		res = m_lpDirectInputDevice->Unacquire();
		return result;
	}
	else 
	{	if (*pdeff == NULL)
			return false;
		else
		{
			HRESULT result = (*pdeff)->Unload();
			(*pdeff)->Release();
			delete *pdeff;
			*pdeff = NULL;
			return result==DI_OK;
		}
	}

	return false;
}

std::string CControllerInput::GetEffectName(EffectType effectType)
{
	return std::string(m_effectInstances[effectType]->tszName);
}


bool CControllerInput::PlayEffect(EffectType effectType,bool stop)
{
	HRESULT result;
	if (m_effects[effectType] != NULL)
	{
		result = m_lpDirectInputDevice->Acquire();
		if (result != DI_OK)
		{
			MessageBox(NULL, "Device not acquired!", "Erreur", MB_OK | MB_ICONERROR);
			return false;
		}
		
		if (!stop)
			result = m_effects[effectType]->Start(1,0);
		else 
			result = m_effects[effectType]->Stop();

		result = m_lpDirectInputDevice->Unacquire();
	}

	return (DI_OK == result);
}


void CControllerInput::InitEffect(LPDIEFFECT effect,EffectType type)
{
	effect->dwSize = sizeof(DIEFFECT);
	effect->dwFlags = DIEFF_CARTESIAN|DIEFF_OBJECTOFFSETS;
	effect->dwDuration = 1000000;//INFINITE;
	effect->dwSamplePeriod = 0;
	effect->dwGain = DI_FFNOMINALMAX;
	effect->dwTriggerButton = DIEB_NOTRIGGER;
	effect->dwTriggerRepeatInterval = 0;

	switch(type)
	{
		case CONSTANTFORCE:
		{	
			m_cstForce.cstForce.lMagnitude = DI_FFNOMINALMAX;
			m_cstForce.axes[0] = DIJOFS_X;
			m_cstForce.direction[0] = 9000;

			effect->cAxes = 1;
			effect->rgdwAxes = m_cstForce.axes;
			effect->rglDirection = m_cstForce.direction;
			effect->lpEnvelope = NULL;
			effect->cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
			effect->lpvTypeSpecificParams = &m_cstForce.cstForce;
			break;
		}
		case RAMPFORCE:
		{	
			break;
		}
		case SQUARE:
		{
			break;
		}
		case SINE:
		{
			effect->cAxes = 1;
			m_periodicForce.periodicForce.dwMagnitude = DI_FFNOMINALMAX;
			m_periodicForce.periodicForce.lOffset = 0;
			m_periodicForce.periodicForce.dwPhase = 0;
			m_periodicForce.periodicForce.dwPeriod = 1000000;
			m_periodicForce.axes[0] = DIJOFS_X;
			m_periodicForce.direction[0] = 9000;
			effect->rgdwAxes = m_periodicForce.axes;
			effect->rglDirection = m_periodicForce.direction;
			effect->lpEnvelope = NULL;
			effect->cbTypeSpecificParams = sizeof(DIPERIODIC);
			effect->lpvTypeSpecificParams = &m_periodicForce.periodicForce;
			break;
		}
		case TRIANGLE:
		{
			break;
		}
		case SAWTOOTHUP:
		{
			break;
		}
		case SAWTOOTHDOWN:
		{
			break;
		}
		case SPRING:
		{
			break;
		}
		case DAMPER:
		{
			break;
		}
		case INERTIA:
		{
			break;
		}
		case FRICTION:
		{
			break;
		}
		case CUSTOMFORCE:
		{
			break;
		}
	}
}