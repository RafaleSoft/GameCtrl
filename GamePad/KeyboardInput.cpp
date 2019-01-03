// KeyboardInput.cpp: implementation of the CKeyboardInput class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ISystem.h"
#include "KeyboardInput.h"

const unsigned char DIK_to_VK_Map[0xE0] = 
{
	0,
	VK_ESCAPE,	//#define DIK_ESCAPE          0x01
	0x31,		//#define DIK_1               0x02
	0x32,		//#define DIK_2               0x03
	0x33,		//#define DIK_3               0x04
	0x34,		//#define DIK_4               0x05
	0x35,		//#define DIK_5               0x06
	0x36,		//#define DIK_6               0x07
	0x37,		//#define DIK_7               0x08
	0x38,		//#define DIK_8               0x09
	0x39,		//#define DIK_9               0x0A
	0x30,		//#define DIK_0               0x0B
	0x29,//#define DIK_MINUS           0x0C    /* - on main keyboard */
	0x3d,//#define DIK_EQUALS          0x0D
	VK_BACK,	//#define DIK_BACK            0x0E    /* backspace */
	VK_TAB,		//#define DIK_TAB             0x0F
	0x51,		//#define DIK_Q               0x10
	0x57,		//#define DIK_W               0x11
	0x45,		//#define DIK_E               0x12
	0x52,		//#define DIK_R               0x13
	0x54,		//#define DIK_T               0x14
	0x59,		//#define DIK_Y               0x15
	0x55,		//#define DIK_U               0x16
	0x49,		//#define DIK_I               0x17
	0x4f,		//#define DIK_O               0x18
	0x50,		//#define DIK_P               0x19
	0x28,//#define DIK_LBRACKET        0x1A
	0x29,//#define DIK_RBRACKET        0x1B
	VK_RETURN,	//#define DIK_RETURN          0x1C    /* Enter on main keyboard */
	VK_LCONTROL,//#define DIK_LCONTROL        0x1D
	0x41,		//#define DIK_A               0x1E
	0x53,		//#define DIK_S               0x1F
	0x44,		//#define DIK_D               0x20
	0x46,		//#define DIK_F               0x21
	0x47,		//#define DIK_G               0x22
	0x48,		//#define DIK_H               0x23
	0x4a,		//#define DIK_J               0x24
	0x4b,		//#define DIK_K               0x25
	0x4c,		//#define DIK_L               0x26
	0,//#define DIK_SEMICOLON       0x27
	0,//#define DIK_APOSTROPHE      0x28
	0,//#define DIK_GRAVE           0x29    /* accent grave */
	VK_LSHIFT,	//#define DIK_LSHIFT          0x2A
	0,//#define DIK_BACKSLASH       0x2B
	0x5a,		//#define DIK_Z               0x2C
	0x58,		//#define DIK_X               0x2D
	0x43,		//#define DIK_C               0x2E
	0x56,		//#define DIK_V               0x2F
	0x42,		//#define DIK_B               0x30
	0x4e,		//#define DIK_N               0x31
	0x4d,		//#define DIK_M               0x32
	0,//#define DIK_COMMA           0x33
	0,//#define DIK_PERIOD          0x34    /* . on main keyboard */
	0,//#define DIK_SLASH           0x35    /* / on main keyboard */
	VK_RSHIFT,	//#define DIK_RSHIFT          0x36
	VK_MULTIPLY,//#define DIK_MULTIPLY        0x37    /* * on numeric keypad */
	VK_LMENU,	//#define DIK_LMENU           0x38    /* left Alt */
	VK_SPACE,	//#define DIK_SPACE           0x39
	VK_CAPITAL,	//#define DIK_CAPITAL         0x3A
	VK_F1,		//#define DIK_F1              0x3B
	VK_F2,		//#define DIK_F2              0x3C
	VK_F3,		//#define DIK_F3              0x3D
	VK_F4,		//#define DIK_F4              0x3E
	VK_F5,		//#define DIK_F5              0x3F
	VK_F6,		//#define DIK_F6              0x40
	VK_F7,		//#define DIK_F7              0x41
	VK_F8,		//#define DIK_F8              0x42
	VK_F9,		//#define DIK_F9              0x43
	VK_F10,		//#define DIK_F10             0x44
	VK_NUMLOCK,	//#define DIK_NUMLOCK         0x45
	VK_SCROLL,	//#define DIK_SCROLL          0x46    /* Scroll Lock */
	VK_NUMPAD7,	//#define DIK_NUMPAD7         0x47
	VK_NUMPAD8,	//#define DIK_NUMPAD8         0x48
	VK_NUMPAD9,	//#define DIK_NUMPAD9         0x49
	VK_SUBTRACT,//#define DIK_SUBTRACT        0x4A    /* - on numeric keypad */
	VK_NUMPAD4,	//#define DIK_NUMPAD4         0x4B
	VK_NUMPAD5,	//#define DIK_NUMPAD5         0x4C
	VK_NUMPAD6,	//#define DIK_NUMPAD6         0x4D
	VK_ADD,		//#define DIK_ADD             0x4E    /* + on numeric keypad */
	VK_NUMPAD1,	//#define DIK_NUMPAD1         0x4F
	VK_NUMPAD2,	//#define DIK_NUMPAD2         0x50
	VK_NUMPAD3,	//#define DIK_NUMPAD3         0x51
	VK_NUMPAD0,	//#define DIK_NUMPAD0         0x52
	VK_DECIMAL,	//#define DIK_DECIMAL         0x53    /* . on numeric keypad */
	0,// 0x54
	0,// 0x55
	0,// 0x56
	VK_F11,		//#define DIK_F11             0x57
	VK_F12,		//#define DIK_F12             0x58
	0,// 0x59
	0,// 0x5A
	0,// 0x5B
	0,// 0x5C
	0,// 0x5D
	0,// 0x5E
	0,// 0x5F
	0,// 0x60
	0,// 0x61
	0,// 0x62
	0,// 0x63
	VK_F13,		//#define DIK_F13             0x64    /*                     (NEC PC98) */
	VK_F14,		//#define DIK_F14             0x65    /*                     (NEC PC98) */
	VK_F15,		//#define DIK_F15             0x66    /*                     (NEC PC98) */
	0,// 0x67
	0,// 0x68
	0,// 0x69
	0,// 0x6A
	0,// 0x6B
	0,// 0x6C
	0,// 0x6D
	0,// 0x6E
	0,// 0x6F
	VK_KANA,	//#define DIK_KANA            0x70    /* (Japanese keyboard)            */
	0,// 0x71
	0,// 0x72
	0,// 0x73
	0,// 0x74
	0,// 0x75
	0,// 0x76
	0,// 0x77
	0,// 0x78
	VK_CONVERT,	//#define DIK_CONVERT         0x79    /* (Japanese keyboard)            */
	0,// 0x7A
	VK_NONCONVERT,//#define DIK_NOCONVERT       0x7B    /* (Japanese keyboard)            */
	0,// 0x7C
	0,//#define DIK_YEN             0x7D    /* (Japanese keyboard)            */
	0,// 0x7E
	0,// 0x7F
	0,// 0x80
	0,// 0x81
	0,// 0x82
	0,// 0x83
	0,// 0x84
	0,// 0x85
	0,// 0x86
	0,// 0x87
	0,// 0x88
	0,// 0x89
	0,// 0x8A
	0,// 0x8B
	0,// 0x8C
	0,//#define DIK_NUMPADEQUALS    0x8D    /* = on numeric keypad (NEC PC98) */
	0,// 0x8E
	0,// 0x8F
	0,//#define DIK_CIRCUMFLEX      0x90    /* (Japanese keyboard)            */
	0,//#define DIK_AT              0x91    /*                     (NEC PC98) */
	0,//#define DIK_COLON           0x92    /*                     (NEC PC98) */
	0,//#define DIK_UNDERLINE       0x93    /*                     (NEC PC98) */
	VK_KANJI,	//#define DIK_KANJI           0x94    /* (Japanese keyboard)            */
	0,//#define DIK_STOP            0x95    /*                     (NEC PC98) */
	0,//#define DIK_AX              0x96    /*                     (Japan AX) */
	0,//#define DIK_UNLABELED       0x97    /*                        (J3100) */
	0,// 0x98
	0,// 0x99
	0,// 0x9A
	0,// 0x9B
	VK_RETURN,//#define DIK_NUMPADENTER     0x9C    /* Enter on numeric keypad */
	VK_RCONTROL,//#define DIK_RCONTROL        0x9D
	0,// 0x9E
	0,// 0x9F
	0,// 0xA0
	0,// 0xA1
	0,// 0xA2
	0,// 0xA3
	0,// 0xA4
	0,// 0xA5
	0,// 0xA6
	0,// 0xA7
	0,// 0xA8
	0,// 0xA9
	0,// 0xAA
	0,// 0xAB
	0,// 0xAC
	0,// 0xAD
	0,// 0xAE
	0,// 0xAF
	0,// 0xB0
	0,// 0xB1
	0,// 0xB2
	VK_DECIMAL,	//#define DIK_NUMPADCOMMA     0xB3    /* , on numeric keypad (NEC PC98) */
	0,// 0xB4
	VK_DIVIDE,	//#define DIK_DIVIDE          0xB5    /* / on numeric keypad */
	0,// 0xB6
	VK_PRINT,	//#define DIK_SYSRQ           0xB7
	VK_RMENU,	//#define DIK_RMENU           0xB8    /* right Alt */
	0,// 0xB9
	0,// 0xBA
	0,// 0xBB
	0,// 0xBC
	0,// 0xBD
	0,// 0xBE
	0,// 0xBF
	0,// 0xC0
	0,// 0xC1
	0,// 0xC2
	0,// 0xC3
	0,// 0xC4
	VK_PAUSE,	//#define DIK_PAUSE           0xC5    /* Pause */
	0,// 0xC6
	VK_HOME,	//#define DIK_HOME            0xC7    /* Home on arrow keypad */
	VK_UP,		//#define DIK_UP              0xC8    /* UpArrow on arrow keypad */
	VK_PRIOR,	//#define DIK_PRIOR           0xC9    /* PgUp on arrow keypad */
	0,// 0xCA
	VK_LEFT,	//#define DIK_LEFT            0xCB    /* LeftArrow on arrow keypad */
	0,// 0xCC
	VK_RIGHT,	//#define DIK_RIGHT           0xCD    /* RightArrow on arrow keypad */
	0,// 0xCE
	VK_END,		//#define DIK_END             0xCF    /* End on arrow keypad */
	VK_DOWN,	//#define DIK_DOWN            0xD0    /* DownArrow on arrow keypad */
	VK_NEXT,	//#define DIK_NEXT            0xD1    /* PgDn on arrow keypad */
	VK_INSERT,	//#define DIK_INSERT          0xD2    /* Insert on arrow keypad */
	VK_DELETE,	//#define DIK_DELETE          0xD3    /* Delete on arrow keypad */
	0,// 0xD4
	0,// 0xD5
	0,// 0xD6
	0,// 0xD7
	0,// 0xD8
	0,// 0xD9
	0,// 0xDA
	VK_LWIN,	//#define DIK_LWIN            0xDB    /* Left Windows key */
	VK_RWIN,	//#define DIK_RWIN            0xDC    /* Right Windows key */
	VK_APPS,	//#define DIK_APPS            0xDD    /* AppMenu key */
	0,//#define DIK_POWER           0xDE
	0//#define DIK_SLEEP           0xDF
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CKeyboardInput::CKeyboardInput(CISystem *ISystem)
{
	if (CreateDevice(ISystem, DI8DEVTYPE_KEYBOARD))
	{
		m_capabilities.dwSize = sizeof(DIDEVCAPS);
		if (DI_OK != m_lpDirectInputDevice->GetCapabilities(&m_capabilities))
			MessageBox(NULL, "Unable to read Device capabilities !", "Erreur", MB_OK | MB_ICONERROR);

		if (DI_OK != m_lpDirectInputDevice->SetDataFormat(&c_dfDIKeyboard))
			MessageBox(NULL, "Failed to set data format", "Erreur", MB_OK | MB_ICONERROR);

		memset(&m_keyboardState, 0, sizeof(KEYBOARDSTATE));
	}
}

CKeyboardInput::~CKeyboardInput()
{

}

const std::string CKeyboardInput::GetTypeName() const
{
	std::string dname = "";

	if (m_lpDeviceInstance == NULL)
		return "";

	BYTE type = BYTE(m_lpDeviceInstance->dwDevType & 0xFF);
	BYTE subtype = BYTE((m_lpDeviceInstance->dwDevType >> 8) & 0xFF);

	if (type == DI8DEVTYPE_KEYBOARD)
	{
		if (subtype == DI8DEVTYPEKEYBOARD_UNKNOWN)
			dname = "unknown";
		else if (subtype == DI8DEVTYPEKEYBOARD_PCXT)
			dname = "pc xt";
		else if (subtype == DI8DEVTYPEKEYBOARD_OLIVETTI)
			dname = "olivetti";
		else if (subtype == DI8DEVTYPEKEYBOARD_PCAT)
			dname = "pc at";
		else if (subtype == DI8DEVTYPEKEYBOARD_PCENH)
			dname = "pc enhanced";
		else if (subtype == DI8DEVTYPEKEYBOARD_NOKIA1050)
			dname = "nokia 1050";
		else if (subtype == DI8DEVTYPEKEYBOARD_NOKIA9140)
			dname = "nokia 9140";
		else if (subtype == DI8DEVTYPEKEYBOARD_NEC98)
			dname = "nec 98";
		else if (subtype == DI8DEVTYPEKEYBOARD_NEC98LAPTOP)
			dname = "nec 98 laptop";
		else if (subtype == DI8DEVTYPEKEYBOARD_NEC98106)
			dname = "nec 98106";
		else if (subtype == DI8DEVTYPEKEYBOARD_JAPAN106)
			dname = "japan 106";
		else if (subtype == DI8DEVTYPEKEYBOARD_JAPANAX)
			dname = "japan ax";
		else if (subtype == DI8DEVTYPEKEYBOARD_J3100)
			dname = "j3100";

		dname += " keyboard";
		return dname;
	}
	else
		return CDeviceInput::GetTypeName();
}

LPCKEYBOARDSTATE CKeyboardInput::getKeyboardState()
{
	HRESULT result;

	result = m_lpDirectInputDevice->Acquire();
	if ((result != DI_OK)&&(result != S_FALSE))
	{
		MessageBox(NULL, "keyboard not acquired!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}

	result = m_lpDirectInputDevice->Poll();
	if (result != DI_OK)
	{
		MessageBox(NULL, "Keyboard not polled!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}

	result = m_lpDirectInputDevice->GetDeviceState(sizeof(KEYBOARDSTATE),&m_keyboardState);

	if (result != DI_OK)
	{
		MessageBox(NULL, "Device state unavailable!", "Erreur", MB_OK | MB_ICONERROR);
		return NULL;
	}
	else
		return &m_keyboardState;
}

unsigned char CKeyboardInput::getKeyboardData(DWORD key)
{
	if ((key>=0) && (key<256))
	{
		unsigned char k = m_keyboardState[key];
		m_keyboardState[key] = 0;
		return k;
	}
	else
		return 0;
}

DWORD CKeyboardInput::hasKeyboardData(DWORD next) const
{
	DWORD pos = next;
	while ((pos<256) && (m_keyboardState[pos] == 0))
		pos++;

	return pos;
}

WORD CKeyboardInput::readKeyboardBuffer(void)
{
	if (m_buffer.size() > 0)
	{
		WORD key = m_buffer[0];
		m_buffer.erase(m_buffer.begin());

		return key;
	}
	else
		return 0;
}

unsigned char CKeyboardInput::DIK_to_VK(unsigned char key) const
{
	if (key<0xe0)
		return DIK_to_VK_Map[key];
	else
		return 0;
}