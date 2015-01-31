// ProfilerHook.h : Declaration of the CProfilerHook

#pragma once
#include "resource.h"       // main symbols



#include "SpikeProfiler_i.h"

#include "ProfileBase.h"


using namespace ATL;

#define COM_FAIL_MSG_RETURN_ERROR(hr, msg) if (!SUCCEEDED(hr)) { ATLTRACE(msg, hr); return (hr); }
#define COM_FAIL_MSG_RETURN_OTHER(hr, ret, msg) if (!SUCCEEDED(hr)) { ATLTRACE(msg, hr); return (ret); }
#define COM_FAIL_RETURN(hr, ret) if (!SUCCEEDED(hr)) return (ret)

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
	CComQIPtr<ICorProfilerInfo4> m_profilerInfo;

private:
	std::wstring GetModulePath(ModuleID moduleId);
	std::wstring GetModulePath(ModuleID moduleId, AssemblyID *pAssemblyId);
	std::wstring GetAssemblyName(AssemblyID assemblyId);
	BOOL GetTokenAndModule(FunctionID funcId, mdToken& functionToken, ModuleID& moduleId, std::wstring &modulePath, AssemblyID *pAssemblyId);
	std::wstring GetTypeAndMethodName(FunctionID functionId);

	mdMemberRef m_targetLoadProfilerInsteadRef;
	mdMemberRef m_targetPretendWeLoadedFakesProfilerRef;
	HRESULT GetInjectedRef(ModuleID moduleId, mdModuleRef &injectedRef);
	HRESULT GetMsCorlibRef(ModuleID moduleId, mdModuleRef &mscorlibRef);
	mdTypeRef m_objectTypeRef;

public:
	virtual HRESULT STDMETHODCALLTYPE Initialize(
		/* [in] */ IUnknown *pICorProfilerInfoUnk);

	virtual HRESULT STDMETHODCALLTYPE Shutdown(void);

	virtual HRESULT STDMETHODCALLTYPE CProfilerHook::ModuleAttachedToAssembly(
		/* [in] */ ModuleID moduleId,
		/* [in] */ AssemblyID assemblyId);

	virtual HRESULT STDMETHODCALLTYPE JITCompilationStarted(
		/* [in] */ FunctionID functionId,
		/* [in] */ BOOL fIsSafeToBlock);

};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerHook), CProfilerHook)
