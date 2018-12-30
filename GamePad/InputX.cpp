// InputX.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static AFX_EXTENSION_MODULE InputXDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// Remove this if you use lpReserved
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("INPUTX.DLL Initializing!\n");
		
		if (!AfxInitExtensionModule(InputXDLL, hInstance))
			return 0;

		new CDynLinkLibrary(InputXDLL);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("INPUTX.DLL Terminating!\n");
		
		AfxTermExtensionModule(InputXDLL);
	}
	return 1;   
}
