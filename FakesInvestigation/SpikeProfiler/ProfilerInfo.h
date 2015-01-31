// ProfilerInfo.h : Declaration of the CProfilerInfo

#pragma once
#include "resource.h"       // main symbols



#include "SpikeProfiler_i.h"

#include "ProfilerInfoBase.h"

using namespace ATL;

// CProfilerInfoBase

class ATL_NO_VTABLE CProfilerInfo :
	public CComObjectRootEx<CComMultiThreadModel>,
	//public CComCoClass<CProfilerInfo>,
	//public CComObject<IUnknown>,
	public CProfilerInfoBase
{
public:
	CProfilerInfo()
	{
	}

	DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERINFO)


	BEGIN_COM_MAP(CProfilerInfo)
		COM_INTERFACE_ENTRY(ICorProfilerInfo)
		COM_INTERFACE_ENTRY(ICorProfilerInfo2)
		COM_INTERFACE_ENTRY(ICorProfilerInfo3)
		COM_INTERFACE_ENTRY(ICorProfilerInfo4)
	END_COM_MAP()


	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:
	virtual HRESULT STDMETHODCALLTYPE SetEventMask(
		/* [in] */ DWORD dwEvents);
};



//OBJECT_ENTRY_AUTO(__uuidof(ProfilerInfo), CProfilerInfo)
