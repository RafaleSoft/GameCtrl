// MouseInput.h: interface for the CMouseInput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MOUSEINPUT_H__20709D12_B11F_47E0_8B05_A3B9C1454EA3__INCLUDED_)
#define AFX_MOUSEINPUT_H__20709D12_B11F_47E0_8B05_A3B9C1454EA3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "GamePad.h"
#include "DeviceInput.h"


class GAMEPAD_API CMouseInput : public CDeviceInput
{
protected:
	DIMOUSESTATE 	m_mouseState;

public:
	CMouseInput(CISystem *ISystem);
	virtual ~CMouseInput();

	/**	Implements base class. */
	virtual DEVICE_TYPE GetType() const { return DEVICE_MOUSE; };

	/**	Implements base class. */
	virtual const std::string GetTypeName() const;


	LPCDIMOUSESTATE getMouseState();
};

#endif // !defined(AFX_MOUSEINPUT_H__20709D12_B11F_47E0_8B05_A3B9C1454EA3__INCLUDED_)
