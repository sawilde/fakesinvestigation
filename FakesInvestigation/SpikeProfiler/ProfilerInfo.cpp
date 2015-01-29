// ProfilerInfo.cpp : Implementation of CProfilerInfo

#include "stdafx.h"
#include "ProfilerInfo.h"


// CProfilerInfo
HRESULT STDMETHODCALLTYPE CProfilerInfo::GetEventMask(
	/* [out] */ DWORD *pdwEvents){
	ATLTRACE(_T("::GetEventMask(%d)"), &pdwEvents);
	return CProfilerInfoBase::GetEventMask(pdwEvents);
}
