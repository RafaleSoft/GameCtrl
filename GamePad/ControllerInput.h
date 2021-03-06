// ControllerInput.h: interface for the CControllerInput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONTROLLERINPUT_H__C7C923F0_20D9_4206_8960_A69567AA771F__INCLUDED_)
#define AFX_CONTROLLERINPUT_H__C7C923F0_20D9_4206_8960_A69567AA771F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "DeviceInput.h"

class CISystem;


class GAMEPAD_API CControllerInput : public CDeviceInput
{
public :
	enum Event
	{
		BUTTON,
		TRIGGER,
		LSTICK,
		RSTICK,
		POV,
		DPAD,
		UNKNOWN
	};

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

	static const uint32_t MAX_BUTTONS = 128;

public:
	/** Constructor. */
	CControllerInput(CISystem *ISystem);
	/** Destructor. */
	virtual ~CControllerInput();

	/** Implements base class. */
	virtual DEVICE_TYPE GetType() const { return DEVICE_CONTROLLER; };

	/**	Implements base class. */
	virtual const std::string GetTypeName() const;


	/**
	 *	gamepad specific methods.
	 */

	/**	Implements base class. */
	bool FillDeviceBuffer(bool doNotify);

	bool Configure() const
	{
		return (DI_OK == m_lpDirectInputDevice->RunControlPanel(NULL,0));
	};
	
	//! Return true if a button state is pressed.
	bool getButtonState(uint32_t num_button) const;

	//	Force feedback
	bool LoadEffects(EffectType effectType,bool unload = false);
	bool PlayEffect(EffectType effectType,bool stop = false);
	std::string GetEffectName(EffectType effectType);


private:
	//!	The controller current state.
	DIJOYSTATE2	m_controllerState;

	//!	Controller objects (buttons, axis, ...)
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
