// KeyboardInput.h: interface for the CKeyboardInput class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_KEYBOARDINPUT_H__37417D20_E5E4_42E9_AB9D_DF0A9FD7992A__INCLUDED_)
#define AFX_KEYBOARDINPUT_H__37417D20_E5E4_42E9_AB9D_DF0A9FD7992A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "GamePad.h"
#include "DeviceInput.h"


class GAMEPAD_API CKeyboardInput : public CDeviceInput
{
public:
	CKeyboardInput(CISystem *ISystem);
	virtual ~CKeyboardInput();

	/**	Implements base class. */
	virtual DEVICE_TYPE GetType() const { return DEVICE_KEYBOARD; };

	/**	Implements base class. */
	virtual const std::string GetTypeName() const;


	/**
	 *	Keyboard specific methods.
	 */
	
	/**	Implements base class. */
	bool FillDeviceBuffer(bool doNotify);

	//!	Returns the state of key after FillDeviceBuffer is called.
	unsigned char getKeyboardData(DWORD key);

	/**
	 *  Each buffer data consist of:
	 *	a word where the high byte is the key status,
	 *	and the low byte is the key.
	 *	The returned key is removed from the buffer.
	 */
	WORD peekKeyboardBuffer(void);

	/**	
	 * Returns the first position where the keyboard state contains data.
	 * FillDeviceBuffer must be called first.
	 * if the keyboard state is empty, the return value is 256
	 */
	DWORD hasKeyboardData(DWORD next = 0) const;

	//! Translates a DirectInput Key to a Win32 SDK Virtual Key
	unsigned char DIK_to_VK(unsigned char key) const;


protected:
	//! Returns the immediate keyboard state.
	LPCKEYBOARDSTATE getKeyboardState(void);

	//!	The keyboard current state.
	KEYBOARDSTATE 	m_keyboardState;
		
	std::vector<WORD>	m_buffer;

};

#endif // !defined(AFX_KEYBOARDINPUT_H__37417D20_E5E4_42E9_AB9D_DF0A9FD7992A__INCLUDED_)
