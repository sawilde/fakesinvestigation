// ProfilerHook.cpp : Implementation of CProfilerHook

#include "stdafx.h"
#include "ProfilerHook.h"

#define TARGETASSEMBLY L"Microsoft.VisualStudio.TestPlatform.Utilities"
#define TARGETMETHOD L"Microsoft.VisualStudio.TestPlatform.Utilities.DefaultTestExecutorLauncher::LaunchProcess"

//#define TARGETASSEMBLY L"System"
//#define TARGETMETHOD L"System.Diagnostics.Process::Start"

// CProfilerHook
HRESULT STDMETHODCALLTYPE CProfilerHook::Initialize(
	/* [in] */ IUnknown *pICorProfilerInfoUnk)
{
	ATLTRACE(_T("::Initialize"));
	m_profilerInfo = pICorProfilerInfoUnk;
	DWORD dwMask = 0;
	dwMask |= COR_PRF_MONITOR_MODULE_LOADS;			// Controls the ModuleLoad, ModuleUnload, and ModuleAttachedToAssembly callbacks.
	dwMask |= COR_PRF_MONITOR_JIT_COMPILATION;	    // Controls the JITCompilation, JITFunctionPitched, and JITInlining callbacks.
	dwMask |= COR_PRF_DISABLE_INLINING;				// Disables all inlining.
	dwMask |= COR_PRF_DISABLE_OPTIMIZATIONS;		// Disables all code optimizations.
	//dwMask |= COR_PRF_USE_PROFILE_IMAGES;           // Causes the native image search to look for profiler-enhanced images

	COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->SetEventMask(dwMask),
		_T("    ::Initialize(...) => SetEventMask => 0x%X"));

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::Shutdown(void)
{
	ATLTRACE(_T("::Shutdown"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::ModuleAttachedToAssembly(
	/* [in] */ ModuleID moduleId,
	/* [in] */ AssemblyID assemblyId)
{
	std::wstring assemblyName = GetAssemblyName(assemblyId);

	if (TARGETASSEMBLY == GetAssemblyName(assemblyId))
	{
		std::wstring modulePath = GetModulePath(moduleId);
		ATLTRACE(_T("::ModuleAttachedToAssembly(...) => (%X => %s, %X => %s)"),
			moduleId, W2CT(modulePath.c_str()),
			assemblyId, W2CT(assemblyName.c_str()));
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::JITCompilationStarted(
	/* [in] */ FunctionID functionId,
	/* [in] */ BOOL fIsSafeToBlock)
{
	std::wstring modulePath;
	mdToken functionToken;
	ModuleID moduleId;
	AssemblyID assemblyId;

	if (GetTokenAndModule(functionId, functionToken, moduleId, modulePath, &assemblyId))
	{
		if (TARGETASSEMBLY == GetAssemblyName(assemblyId))
		{
			std::wstring typeName = GetTypeAndMethodName(functionId);

			if (TARGETMETHOD == typeName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeName.c_str()));
			}
		}
	}

	return S_OK;
}

std::wstring CProfilerHook::GetModulePath(ModuleID moduleId)
{
	ULONG dwNameSize = 512;
	WCHAR szModulePath[512] = {};
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetModuleInfo(moduleId, NULL, dwNameSize, &dwNameSize, szModulePath, NULL), std::wstring(),
		_T("    ::GetModulePath(ModuleID) => GetModuleInfo => 0x%X"));
	return std::wstring(szModulePath);
}

std::wstring CProfilerHook::GetModulePath(ModuleID moduleId, AssemblyID *pAssemblyID)
{
	ULONG dwNameSize = 512;
	WCHAR szModulePath[512] = {};
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetModuleInfo(moduleId, NULL, dwNameSize, &dwNameSize, szModulePath, pAssemblyID), std::wstring(),
		_T("    ::GetModulePath(ModuleID,AssemblyID*) => GetModuleInfo => 0x%X"));
	return std::wstring(szModulePath);
}

std::wstring CProfilerHook::GetAssemblyName(AssemblyID assemblyId)
{
	ULONG dwNameSize = 512;
	WCHAR szAssemblyName[512] = {};
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetAssemblyInfo(assemblyId, dwNameSize, &dwNameSize, szAssemblyName, NULL, NULL), std::wstring(),
		_T("    ::GetAssemblyName(AssemblyID) => GetAssemblyInfo => 0x%X"));
	return std::wstring(szAssemblyName);
}

BOOL CProfilerHook::GetTokenAndModule(FunctionID funcId, mdToken& functionToken, ModuleID& moduleId, std::wstring &modulePath, AssemblyID *pAssemblyId)
{
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetFunctionInfo2(funcId, NULL, NULL, &moduleId, &functionToken, 0, NULL, NULL), FALSE,
		_T("    ::GetTokenAndModule(...) => GetFunctionInfo2 => 0x%X"));
	modulePath = GetModulePath(moduleId, pAssemblyId);
	return TRUE;
}

std::wstring CProfilerHook::GetTypeAndMethodName(FunctionID functionId)
{
	std::wstring empty = L"";
	CComPtr<IMetaDataImport2> metaDataImport2;
	mdMethodDef functionToken;
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetTokenAndMetaDataFromFunction(functionId, IID_IMetaDataImport, (IUnknown **)&metaDataImport2, &functionToken),
		empty, _T("GetTokenAndMetaDataFromFunction"));

	mdTypeDef classId;
	WCHAR szMethodName[512] = {};
	COM_FAIL_MSG_RETURN_OTHER(metaDataImport2->GetMethodProps(functionToken, &classId, szMethodName, 512, NULL, NULL, NULL, NULL, NULL, NULL),
		empty, _T("GetMethodProps"));

	WCHAR szTypeName[512] = {};
	COM_FAIL_MSG_RETURN_OTHER(metaDataImport2->GetTypeDefProps(classId, szTypeName, 512, NULL, NULL, NULL),
		empty, _T("GetTypeDefProps"));

	std::wstring methodName = szTypeName;
	methodName += L"::";
	methodName += szMethodName;

	//ATLTRACE(_T("::GetTypeAndMethodName(%s)"), W2CT(methodName.c_str()));

	return methodName;
}