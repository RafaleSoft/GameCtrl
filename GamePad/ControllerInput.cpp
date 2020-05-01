// ControllerInput.cpp: implementation of the CControllerInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
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

	if (CreateDevice(ISystem, DI8DEVTYPE_GAMEPAD))
	{
		DIPROPDWORD property;
		property.diph.dwSize = sizeof(DIPROPDWORD);
		property.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		property.diph.dwHow = DIPH_DEVICE;
		property.diph.dwObj = 0;
		property.dwData = DIPROPAUTOCENTER_OFF;

		HRESULT res = m_lpDirectInputDevice->SetProperty(DIPROP_AUTOCENTER, &(property.diph));
		//if (DI_OK != res)
		//	MessageBox(NULL, "Autocenter capability is not available", "Error", MB_OK | MB_ICONERROR);

		res = m_lpDirectInputDevice->SetDataFormat(&c_dfDIJoystick2);
		if (DI_OK != res)
			MessageBox(NULL, "Failed to set data format", "Erreur", MB_OK | MB_ICONERROR);

		res = m_lpDirectInputDevice->EnumEffects(&DIEnumEffectsProc, &m_effectInstances, DIEFT_ALL);
		if (DI_OK != res)
			MessageBox(NULL, "Failed to enumerate effects", "Erreur", MB_OK | MB_ICONERROR);

		res = m_lpDirectInputDevice->EnumObjects(DIEnumDeviceObjectsProc, &m_objects, DIDFT_ALL);
		if (DI_OK != res)
			MessageBox(NULL, "Failed to read controller objects", "Erreur", MB_OK | MB_ICONERROR);
		else
		{
			m_cstForce.axes = new DWORD[1];
			m_cstForce.direction = new LONG[1];
			m_periodicForce.axes = new DWORD[1];
			m_periodicForce.direction = new LONG[1];
		}

		memset(&m_controllerState, 0, sizeof(DIJOYSTATE2));
	}
}

CControllerInput::~CControllerInput()
{	
	for (size_t i=0;i<m_effectInstances.size();i++)
		delete ((LPDIEFFECTINFO)(m_effectInstances[i]));

	m_effectInstances.clear();

	for (unsigned int i=0; i<NB_FF_EFFECTS; i++)
		if (m_effects[i] != NULL)
			m_effects[i]->Release();
}


const std::string CControllerInput::GetTypeName() const
{
	std::string dname = "";

	if (m_lpDeviceInstance == NULL)
		return "";

	BYTE type = BYTE(m_lpDeviceInstance->dwDevType & 0xFF);
	BYTE subtype = BYTE((m_lpDeviceInstance->dwDevType >> 8) & 0xFF);

	if (type == DI8DEVTYPE_JOYSTICK)
	{
		if (subtype == DI8DEVTYPEJOYSTICK_LIMITED)
			dname = "limited";
		else if (subtype == DI8DEVTYPEJOYSTICK_STANDARD)
			dname = "standard";

		dname += " joystick";
		return dname;
	}
	else if (type == DI8DEVTYPE_GAMEPAD)
	{
		if (subtype == DI8DEVTYPEGAMEPAD_LIMITED)
			dname = "limited";
		else if (subtype == DI8DEVTYPEGAMEPAD_STANDARD)
			dname = "standard";
		else if (subtype == DI8DEVTYPEGAMEPAD_TILT)
			dname = "tilt";

		dname += " gamepad";
		return dname;
	}
	else
		return CDeviceInput::GetTypeName();
}

bool CControllerInput::getButtonState(uint32_t num_button) const
{
	if (NULL == m_lpDirectInputDevice)
		return false;
	else if (num_button > MAX_BUTTONS)
		return false;
	else	
		return (m_controllerState.rgbButtons[num_button] != 0);
}

bool CControllerInput::FillDeviceBuffer(bool doNotify)
{
	if (NULL == m_lpDirectInputDevice)
		return false;

	LPCDIJOYSTATE2 data = (LPCDIJOYSTATE2)getDeviceState(sizeof(DIJOYSTATE2), &m_controllerState);
	if (data != NULL)
	{
		if (doNotify)
		{
			for (size_t i = 0; i < m_actions.size(); i++)
			{
				for (size_t j = 0; j < MAX_BUTTONS; j++)
					if (m_controllerState.rgbButtons[j] != 0)
						m_actions[i]->execute(BUTTON, j);
				
				//if (((m_controllerState.lRx != 0x7fff) && (m_controllerState.lRx != 0x8000)) ||
				//	((m_controllerState.lRy != 0x7fff) && (m_controllerState.lRy != 0x8000)))
				{
					DWORD dx = (m_controllerState.lRx & 0x0000FFFF);
					DWORD dy = (m_controllerState.lRy & 0x0000FFFF) << 16;
					m_actions[i]->execute(RSTICK, dx | dy);
				}
				
				//if ((m_controllerState.lX != 0x7fff) || (m_controllerState.lY != 0x7fff))
				{
					DWORD dx = (m_controllerState.lX & 0x0000FFFF);
					DWORD dy = (m_controllerState.lY & 0x0000FFFF) << 16;
					m_actions[i]->execute(LSTICK, dx | dy);
				}

				if (m_controllerState.rgdwPOV[0] != (DWORD)-1)
					m_actions[i]->execute(DPAD, m_controllerState.rgdwPOV[0]);
			}
		}
		return true;
	}
	else
		return false;
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