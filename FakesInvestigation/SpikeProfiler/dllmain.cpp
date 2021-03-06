// dllmain.cpp : Implementation of DllMain.

#include "stdafx.h"
#include "resource.h"
#include "SpikeProfiler_i.h"
#include "dllmain.h"
#include "xdlldata.h"

CSpikeProfilerModule _AtlModule;

HINSTANCE g_hInst = 0;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
#ifdef _MERGE_PROXYSTUB
	if (!PrxDllMain(hInstance, dwReason, lpReserved))
		return FALSE;
#endif
	g_hInst = hInstance;
	return _AtlModule.DllMain(dwReason, lpReserved); 
}
