// GamePad.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "GamePad.h"


// This is an example of an exported variable
GAMEPAD_API int nGamePad=0;

// This is an example of an exported function.
GAMEPAD_API int fnGamePad(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see GamePad.h for the class definition
CGamePad::CGamePad()
{
	return;
}
