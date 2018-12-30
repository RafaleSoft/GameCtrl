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

	virtual DEVICE_TYPE GetType() const { return DEVICE_KEYBOARD; };

	LPCKEYBOARDSTATE getKeyboardState();

	//	returns the state of key
	unsigned char getKeyboardData(DWORD key);

	//	each buffer data consist of
	//	a word where the high byte
	//	is the key status
	//	and the low byte is the
	//	key
	WORD readKeyboardBuffer(void);

	//	returns the first position where
	//	the keyboard state contains data.
	//	if the keyboard state is empty,
	//	the return value is 256
	DWORD hasKeyboardData(DWORD next = 0) const;

	//	translates a DirectInput Key to
	//	a Win32 SDK Virtual Key
	unsigned char DIK_to_VK(unsigned char key) const;

protected:
	friend UINT Poller( LPVOID pParam );

	KEYBOARDSTATE 	m_keyboardState;
		
	std::vector<WORD>	m_buffer;

};

#endif // !defined(AFX_KEYBOARDINPUT_H__37417D20_E5E4_42E9_AB9D_DF0A9FD7992A__INCLUDED_)
