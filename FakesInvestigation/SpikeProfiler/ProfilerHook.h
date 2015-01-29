// ProfilerHook.h : Declaration of the CProfilerHook

#pragma once
#include "resource.h"       // main symbols



#include "SpikeProfiler_i.h"

#include "ProfileBase.h"


using namespace ATL;


// CProfilerHook

class ATL_NO_VTABLE CProfilerHook :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CProfilerHook, &CLSID_ProfilerHook>,
	public CProfilerBase
{
public:
	CProfilerHook()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PROFILERHOOK)


BEGIN_COM_MAP(CProfilerHook)
	COM_INTERFACE_ENTRY(ICorProfilerCallback)
	COM_INTERFACE_ENTRY(ICorProfilerCallback2)
	COM_INTERFACE_ENTRY(ICorProfilerCallback3)
#ifndef _TOOLSETV71
	COM_INTERFACE_ENTRY(ICorProfilerCallback4)
	COM_INTERFACE_ENTRY(ICorProfilerCallback5)
#endif
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

public:
	virtual HRESULT STDMETHODCALLTYPE Initialize(
		/* [in] */ IUnknown *pICorProfilerInfoUnk);

	virtual HRESULT STDMETHODCALLTYPE Shutdown(void);

};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerHook), CProfilerHook)
