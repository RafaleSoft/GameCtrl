// ControllerInput.h: interface for the CControllerInput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTROLLERINPUT_H__C7C923F0_20D9_4206_8960_A69567AA771F__INCLUDED_)
#define AFX_CONTROLLERINPUT_H__C7C923F0_20D9_4206_8960_A69567AA771F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <vector>
#include "DeviceInput.h"
class CISystem;


class GAMEPAD_API CControllerInput : public CDeviceInput
{
public :
	//	Force Feedback
	enum EffectType
	{	
		CONSTANTFORCE,
		RAMPFORCE,
		SQUARE,
		SINE,
		TRIANGLE,
		SAWTOOTHUP,
		SAWTOOTHDOWN,
		SPRING,
		DAMPER,
		INERTIA,
		FRICTION,
		CUSTOMFORCE,
		NB_FF_EFFECTS,
	};

	typedef struct CONSTANTFORCESETTINGS_t
	{
		DICONSTANTFORCE		cstForce;
		DWORD				*axes;
		LONG				*direction;
	} CONSTANTFORCESETTINGS;

	typedef struct PERIODICFORCESETTINGS_t
	{
		DIPERIODIC			periodicForce;
		DWORD				*axes;
		LONG				*direction;
	} PERIODICFORCESETTINGS;


public:
	/** Constructor. */
	CControllerInput(CISystem *ISystem);
	/** Destructor. */
	virtual ~CControllerInput();

	/** Returns the type of this device. */
	virtual DEVICE_TYPE GetType() const { return DEVICE_CONTROLLER; };

	bool Configure() const
	{
		return (DI_OK == m_lpDirectInputDevice->RunControlPanel(NULL,0));
	};
	
	LPCDIJOYSTATE2 getControllerState();

	//	Force feedback
	bool LoadEffects(EffectType effectType,bool unload = false);
	bool PlayEffect(EffectType effectType,bool stop = false);
	std::string GetEffectName(EffectType effectType);


protected:
	DIJOYSTATE2	m_controllerState;


private:
	//	Controller objects
	std::vector<LPDIDEVICEOBJECTINSTANCE>	m_objects;

	CONSTANTFORCESETTINGS m_cstForce;
	PERIODICFORCESETTINGS m_periodicForce;

	//	force feedback management
	std::vector<LPCDIEFFECTINFO>	m_effectInstances;
	LPDIRECTINPUTEFFECT				m_effects[NB_FF_EFFECTS];
	LPDIEFFECT						m_effectsSettings[NB_FF_EFFECTS];

	void	InitEffect(LPDIEFFECT effect,EffectType type);


};

#endif // !defined(AFX_CONTROLLERINPUT_H__C7C923F0_20D9_4206_8960_A69567AA771F__INCLUDED_)
