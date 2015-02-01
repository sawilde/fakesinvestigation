// ProfilerHook.h : Declaration of the CProfilerHook

#pragma once
#include "resource.h"       // main symbols



#include "SpikeProfiler_i.h"

#include "ProfileBase.h"
#include "ProfilerInfo.h"
#include "Instruction.h"

extern HINSTANCE g_hInst;

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

	DWORD AppendProfilerEventMask(DWORD currentEventMask);

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

	CComObject<CProfilerInfo> *m_infoHook;

	CComPtr<ICorProfilerCallback4> m_chainedProfiler;

	mdSignature GetMethodSignatureToken_OBJ(ModuleID moduleID);
	mdMethodDef Get_CurrentDomainMethod(ModuleID moduleID);

	mdMethodDef m_pinvokeAttach;
	mdMethodDef CreatePInvokeHook(IMetaDataEmit* pMetaDataEmit);

	HRESULT InstrumentMethodWith(ModuleID moduleId, mdToken functionToken, InstructionList &instructions);

private:
	HRESULT GetModuleRef(ModuleID moduleId, WCHAR*moduleName, mdModuleRef &mscorlibRef);

	HRESULT GetModuleRef4000(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef);
	HRESULT GetModuleRef2000(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef);
	HRESULT GetModuleRef2050(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef);

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

public:
	// COR_PRF_MONITOR_APPDOMAIN_LOADS
	virtual HRESULT STDMETHODCALLTYPE AppDomainCreationStarted(
		/* [in] */ AppDomainID appDomainId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AppDomainCreationStarted(appDomainId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AppDomainCreationFinished(
		/* [in] */ AppDomainID appDomainId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AppDomainCreationFinished(appDomainId,hrStatus);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted(
		/* [in] */ AppDomainID appDomainId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AppDomainShutdownStarted(appDomainId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished(
		/* [in] */ AppDomainID appDomainId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AppDomainShutdownFinished(appDomainId,hrStatus);
		return S_OK;
	}

	// COR_PRF_MONITOR_ASSEMBLY_LOADS
	virtual HRESULT STDMETHODCALLTYPE AssemblyLoadStarted(
		/* [in] */ AssemblyID assemblyId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AssemblyLoadStarted(assemblyId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AssemblyLoadFinished(
		/* [in] */ AssemblyID assemblyId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AssemblyLoadFinished(assemblyId,hrStatus);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted(
		/* [in] */ AssemblyID assemblyId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AssemblyUnloadStarted(assemblyId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished(
		/* [in] */ AssemblyID assemblyId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->AssemblyUnloadFinished(assemblyId,hrStatus);
		return S_OK;
	}

	// COR_PRF_MONITOR_MODULE_LOADS
	virtual HRESULT STDMETHODCALLTYPE ModuleLoadStarted(
		/* [in] */ ModuleID moduleId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ModuleLoadStarted(moduleId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ModuleLoadFinished(
		/* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ModuleLoadFinished(moduleId, hrStatus);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ModuleUnloadStarted(
		/* [in] */ ModuleID moduleId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ModuleUnloadStarted(moduleId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ModuleUnloadFinished(
		/* [in] */ ModuleID moduleId,
		/* [in] */ HRESULT hrStatus)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ModuleUnloadFinished(moduleId, hrStatus);
		return S_OK;
	}

	//virtual HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly(
	//	/* [in] */ ModuleID moduleId,
	//	/* [in] */ AssemblyID assemblyId)
	//{
	//	return S_OK;
	//}

	//COR_PRF_MONITOR_JIT_COMPILATION
	//virtual HRESULT STDMETHODCALLTYPE JITCompilationStarted(
	//	/* [in] */ FunctionID functionId,
	//	/* [in] */ BOOL fIsSafeToBlock)
	//{
	//	return S_OK;
	//}

	virtual HRESULT STDMETHODCALLTYPE JITCompilationFinished(
		/* [in] */ FunctionID functionId,
		/* [in] */ HRESULT hrStatus,
		/* [in] */ BOOL fIsSafeToBlock)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->JITCompilationFinished(functionId, hrStatus, fIsSafeToBlock);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE JITFunctionPitched(
		/* [in] */ FunctionID functionId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->JITFunctionPitched(functionId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE JITInlining(
		/* [in] */ FunctionID callerId,
		/* [in] */ FunctionID calleeId,
		/* [out] */ BOOL *pfShouldInline)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->JITInlining(callerId, calleeId, pfShouldInline);
		return S_OK;
	}

	// COR_PRF_MONITOR_THREADS
	virtual HRESULT STDMETHODCALLTYPE ThreadCreated(
		/* [in] */ ThreadID threadId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ThreadCreated(threadId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ThreadDestroyed(
		/* [in] */ ThreadID threadId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ThreadDestroyed(threadId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread(
		/* [in] */ ThreadID managedThreadId,
		/* [in] */ DWORD osThreadId)
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ThreadAssignedToOSThread(managedThreadId, osThreadId);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ThreadNameChanged(
		/* [in] */ ThreadID threadId,
		/* [in] */ ULONG cchName,
		/* [in] */
		__in_ecount_opt(cchName)  WCHAR name[])
	{
		if (m_chainedProfiler != NULL)
			return m_chainedProfiler->ThreadNameChanged(threadId, cchName, name);
		return S_OK;
	}
};

OBJECT_ENTRY_AUTO(__uuidof(ProfilerHook), CProfilerHook)
