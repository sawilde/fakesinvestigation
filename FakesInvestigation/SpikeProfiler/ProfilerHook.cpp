// ProfilerHook.cpp : Implementation of CProfilerHook

#include "stdafx.h"
#include "ProfilerHook.h"


// CProfilerHook
HRESULT STDMETHODCALLTYPE CProfilerHook::Initialize(
	/* [in] */ IUnknown *pICorProfilerInfoUnk)
{
	ATLTRACE(_T("::Initialize"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::Shutdown(void)
{
	ATLTRACE(_T("::Shutdown"));
	return S_OK;
}