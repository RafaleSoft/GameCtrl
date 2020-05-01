// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GAMEPAD_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GAMEPAD_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GAMEPAD_EXPORTS
#define GAMEPAD_API __declspec(dllexport)
#else
#define GAMEPAD_API __declspec(dllimport)
#endif


#include <vector>
#include <string>
#include <stdint.h>


#ifndef DIRECTINPUT_VERSION
	#define DIRECTINPUT_VERSION 0x0800
#endif

#include <dinput.h>

//	Extended constant joystick state
typedef DIJOYSTATE2 const * const LPCDIJOYSTATE2;

//	Keyboard state 
typedef unsigned char KEYBOARDSTATE[256];
typedef KEYBOARDSTATE const * const LPCKEYBOARDSTATE;

//	Mouse state
typedef DIMOUSESTATE const * const LPCDIMOUSESTATE;
