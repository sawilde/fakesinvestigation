// ProfilerHook.cpp : Implementation of CProfilerHook

#include "stdafx.h"
#include "ProfilerHook.h"
#include "Method.h"
#include "ProfilerInfo.h"

#define TARGETASSEMBLY1 L"Microsoft.VisualStudio.TestPlatform.Utilities"
#define TARGETMETHOD1 L"Microsoft.VisualStudio.TestPlatform.Utilities.DefaultTestExecutorLauncher::LaunchProcess"

#define TARGETASSEMBLY2A L"vstest.executionengine"
#define TARGETASSEMBLY2B L"vstest.executionengine.x86"
#define TARGETMETHOD2 L"Microsoft.VisualStudio.TestPlatform.TestExecutor.ServiceMain::Main"

// CProfilerHook
HRESULT STDMETHODCALLTYPE CProfilerHook::Initialize(
	/* [in] */ IUnknown *pICorProfilerInfoUnk)
{
	ATLTRACE(_T("::Initialize"));

	TCHAR ext[1024] = { 0 };

	m_profilerInfo = pICorProfilerInfoUnk;
	::GetEnvironmentVariable(_T("CHAIN_EXTERNAL_PROFILER"), ext, 1024);
	if (ext[0] != 0)
	{
		ATLTRACE(_T("    ::Initialize(...) => ext = %s"), ext);
		
		TCHAR loc[1024] = { 0 };
		::GetEnvironmentVariable(_T("CHAIN_EXTERNAL_PROFILER_LOCATION"), loc, 1024);
		ATLTRACE(_T("    ::Initialize(...) => loc = %s"), loc);

		CLSID clsid;
		HRESULT hr = CLSIDFromString(T2OLE(ext), &clsid);
		ATLASSERT(hr == S_OK);

		HMODULE hmodule = LoadLibrary(loc);
		ATLASSERT(hmodule != NULL);

		BOOL(WINAPI*DllGetClassObject)(REFCLSID, REFIID, LPVOID) =
			(BOOL(WINAPI*)(REFCLSID, REFIID, LPVOID))GetProcAddress(hmodule, "DllGetClassObject");
		ATLASSERT(DllGetClassObject != NULL);

		CComPtr<IClassFactory> pClassFactory;
		hr = DllGetClassObject(clsid, IID_IClassFactory, &pClassFactory);
		ATLASSERT(hr == S_OK);

		
		hr = pClassFactory->CreateInstance(NULL, __uuidof(ICorProfilerCallback4), (void**)&m_chainedProfiler);
		ATLASSERT(hr == S_OK);

		HRESULT hr2 = CComObject<CProfilerInfo>::CreateInstance(&m_infoHook);
		ULONG count = m_infoHook->AddRef();

		m_infoHook->m_pProfilerHook = this;

		m_infoHook->SetProfilerInfo(pICorProfilerInfoUnk);

		hr = m_chainedProfiler->Initialize(m_infoHook);
		ATLTRACE(_T("  ::Initialize => fakes = 0x%X"), hr);
	}
	else
	{
		DWORD dwMask = AppendProfilerEventMask(0);
		COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->SetEventMask(dwMask),
			_T("    ::Initialize(...) => SetEventMask => 0x%X"));
	}

	return S_OK;
}

DWORD CProfilerHook::AppendProfilerEventMask(DWORD currentEventMask)
{
	DWORD dwMask = currentEventMask;
	dwMask |= COR_PRF_MONITOR_MODULE_LOADS;			// Controls the ModuleLoad, ModuleUnload, and ModuleAttachedToAssembly callbacks.
	dwMask |= COR_PRF_MONITOR_JIT_COMPILATION;	    // Controls the JITCompilation, JITFunctionPitched, and JITInlining callbacks.
	dwMask |= COR_PRF_DISABLE_INLINING;				// Disables all inlining.
	dwMask |= COR_PRF_DISABLE_OPTIMIZATIONS;		// Disables all code optimizations.
	dwMask |= COR_PRF_USE_PROFILE_IMAGES;           // Causes the native image search to look for profiler-enhanced images
	return dwMask;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::Shutdown(void)
{
	ATLTRACE(_T("::Shutdown"));
	if (m_chainedProfiler != NULL)
		m_chainedProfiler->Shutdown();
	return S_OK;
}

HRESULT CProfilerHook::GetInjectedRef(ModuleID moduleId, mdModuleRef &mscorlibRef)
{
	// get interfaces
	CComPtr<IMetaDataEmit2> metaDataEmit;
	COM_FAIL_RETURN(m_profilerInfo->GetModuleMetaData(moduleId,
		ofRead | ofWrite, IID_IMetaDataEmit2, (IUnknown**)&metaDataEmit), S_OK);

	CComPtr<IMetaDataAssemblyEmit> metaDataAssemblyEmit;
	COM_FAIL_RETURN(metaDataEmit->QueryInterface(
		IID_IMetaDataAssemblyEmit, (void**)&metaDataAssemblyEmit), S_OK);

	// find injected
	ASSEMBLYMETADATA assembly;
	ZeroMemory(&assembly, sizeof(assembly));
	assembly.usMajorVersion = 1;
	assembly.usMinorVersion = 0;
	assembly.usBuildNumber = 0;
	assembly.usRevisionNumber = 0;
	const BYTE pubToken[] = { 0xe1, 0x91, 0x8c, 0xac, 0x69, 0xeb, 0x73, 0xf4 };
	COM_FAIL_RETURN(metaDataAssemblyEmit->DefineAssemblyRef(pubToken,
		sizeof(pubToken), L"Injected", &assembly, NULL, 0, 0,
		&mscorlibRef), S_OK);
}

HRESULT CProfilerHook::GetMsCorlibRef(ModuleID moduleId, mdModuleRef &mscorlibRef)
{
	// get interfaces
	CComPtr<IMetaDataEmit> metaDataEmit;
	COM_FAIL_RETURN(m_profilerInfo->GetModuleMetaData(moduleId,
		ofRead | ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit), S_OK);

	CComPtr<IMetaDataAssemblyEmit> metaDataAssemblyEmit;
	COM_FAIL_RETURN(metaDataEmit->QueryInterface(
		IID_IMetaDataAssemblyEmit, (void**)&metaDataAssemblyEmit), S_OK);

	// find mscorlib
	ASSEMBLYMETADATA assembly;
	ZeroMemory(&assembly, sizeof(assembly));
	assembly.usMajorVersion = 4;
	assembly.usMinorVersion = 0;
	assembly.usBuildNumber = 0;
	assembly.usRevisionNumber = 0;
	BYTE publicKey[] = { 0xB7, 0x7A, 0x5C, 0x56, 0x19, 0x34, 0xE0, 0x89 };
	COM_FAIL_RETURN(metaDataAssemblyEmit->DefineAssemblyRef(publicKey,
		sizeof(publicKey), L"mscorlib", &assembly, NULL, 0, 0,
		&mscorlibRef), S_OK);
}


HRESULT STDMETHODCALLTYPE CProfilerHook::ModuleAttachedToAssembly(
	/* [in] */ ModuleID moduleId,
	/* [in] */ AssemblyID assemblyId)
{
	if (m_chainedProfiler != NULL)
		m_chainedProfiler->ModuleAttachedToAssembly(moduleId, assemblyId);

	std::wstring assemblyName = GetAssemblyName(assemblyId);
	/*std::wstring modulePath = GetModulePath(moduleId);
	ATLTRACE(_T("::ModuleAttachedToAssembly(...) => (%X => %s, %X => %s)"),
		moduleId, W2CT(modulePath.c_str()),
		assemblyId, W2CT(assemblyName.c_str()));*/

	if (TARGETASSEMBLY1 == GetAssemblyName(assemblyId))
	{
		std::wstring modulePath = GetModulePath(moduleId);
		ATLTRACE(_T("::ModuleAttachedToAssembly(...) => (%X => %s, %X => %s)"),
			moduleId, W2CT(modulePath.c_str()),
			assemblyId, W2CT(assemblyName.c_str()));

		m_targetLoadProfilerInsteadRef = 0;
		// get reference to injected
		mdModuleRef injectedRef;
		COM_FAIL_RETURN(GetInjectedRef(moduleId, injectedRef), S_OK);

		// get interfaces
		CComPtr<IMetaDataEmit> metaDataEmit;
		COM_FAIL_RETURN(m_profilerInfo->GetModuleMetaData(moduleId,
			ofRead | ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit), S_OK);

		static COR_SIGNATURE methodCallSignature[] =
		{
			IMAGE_CEE_CS_CALLCONV_DEFAULT,
			0x01,
			ELEMENT_TYPE_VOID,
			ELEMENT_TYPE_OBJECT
		};

		// get method to call
		mdTypeRef classTypeRef;
		COM_FAIL_RETURN(metaDataEmit->DefineTypeRefByName(injectedRef,
			L"Injected.FakesHelper", &classTypeRef), S_OK);
		COM_FAIL_RETURN(metaDataEmit->DefineMemberRef(classTypeRef,
			L"LoadProfilerInstead", methodCallSignature,
			sizeof(methodCallSignature), &m_targetLoadProfilerInsteadRef), S_OK);

		// get object ref
		mdModuleRef mscorlibRef;
		COM_FAIL_RETURN(GetMsCorlibRef(moduleId, mscorlibRef), S_OK);
		COM_FAIL_RETURN(metaDataEmit->DefineTypeRefByName(mscorlibRef,
			L"System.Object", &m_objectTypeRef), S_OK);

	}

	if (TARGETASSEMBLY2B == GetAssemblyName(assemblyId))
	{
		std::wstring modulePath = GetModulePath(moduleId);
		ATLTRACE(_T("::ModuleAttachedToAssembly(...) => (%X => %s, %X => %s)"),
			moduleId, W2CT(modulePath.c_str()),
			assemblyId, W2CT(assemblyName.c_str()));

		m_targetPretendWeLoadedFakesProfilerRef = 0;
		// get reference to injected
		mdModuleRef injectedRef;
		COM_FAIL_RETURN(GetInjectedRef(moduleId, injectedRef), S_OK);

		// get interfaces
		CComPtr<IMetaDataEmit> metaDataEmit;
		COM_FAIL_RETURN(m_profilerInfo->GetModuleMetaData(moduleId,
			ofRead | ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit), S_OK);

		static COR_SIGNATURE methodCallSignature[] =
		{
			IMAGE_CEE_CS_CALLCONV_DEFAULT,
			0x01,
			ELEMENT_TYPE_VOID,
			ELEMENT_TYPE_OBJECT
		};

		// get method to call
		mdTypeRef classTypeRef;
		COM_FAIL_RETURN(metaDataEmit->DefineTypeRefByName(injectedRef,
			L"Injected.FakesHelper", &classTypeRef), S_OK);
		COM_FAIL_RETURN(metaDataEmit->DefineMemberRef(classTypeRef,
			L"PretendWeLoadedFakesProfiler", methodCallSignature,
			sizeof(methodCallSignature), &m_targetPretendWeLoadedFakesProfilerRef), S_OK);

		// get object ref
		mdModuleRef mscorlibRef;
		COM_FAIL_RETURN(GetMsCorlibRef(moduleId, mscorlibRef), S_OK);
		COM_FAIL_RETURN(metaDataEmit->DefineTypeRefByName(mscorlibRef,
			L"System.Object", &m_objectTypeRef), S_OK);

	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::JITCompilationStarted(
	/* [in] */ FunctionID functionId,
	/* [in] */ BOOL fIsSafeToBlock)
{
	if (m_chainedProfiler != NULL)
		m_chainedProfiler->JITCompilationStarted(functionId, fIsSafeToBlock);

	std::wstring modulePath;
	mdToken functionToken;
	ModuleID moduleId;
	AssemblyID assemblyId;

	if (GetTokenAndModule(functionId, functionToken, moduleId, modulePath, &assemblyId))
	{
		if (TARGETASSEMBLY1 == GetAssemblyName(assemblyId))
		{
			std::wstring typeName = GetTypeAndMethodName(functionId);

			if (TARGETMETHOD1 == typeName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeName.c_str()));
			
				LPCBYTE pMethodHeader = NULL;
				ULONG iMethodSize = 0;
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBody(moduleId, functionToken, &pMethodHeader, &iMethodSize),
					_T("    ::JITCompilationStarted(...) => GetILFunctionBody => 0x%X"));

				IMAGE_COR_ILMETHOD* pMethod = (IMAGE_COR_ILMETHOD*)pMethodHeader;

				Method instumentedMethod(pMethod);
				InstructionList instructions; // NOTE: this IL will be different for an instance method or if the local vars signature is different
				instructions.push_back(new Instruction(CEE_LDARG_S, 4));
				instructions.push_back(new Instruction(CEE_CALL, m_targetLoadProfilerInsteadRef));

				instumentedMethod.InsertInstructionsAtOriginalOffset(0, instructions);

				//instumentedMethod.DumpIL();

				// now to write the method back
				CComPtr<IMethodMalloc> methodMalloc;
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBodyAllocator(moduleId, &methodMalloc),
					_T("    ::JITCompilationStarted(...) => GetILFunctionBodyAllocator=> 0x%X"));
				IMAGE_COR_ILMETHOD* pNewMethod = (IMAGE_COR_ILMETHOD*)methodMalloc->Alloc(instumentedMethod.GetMethodSize());
				instumentedMethod.WriteMethod(pNewMethod);
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->SetILFunctionBody(moduleId, functionToken, (LPCBYTE)pNewMethod),
					_T("    ::JITCompilationStarted(...) => SetILFunctionBody => 0x%X"));
			}
		}

		if (TARGETASSEMBLY2B == GetAssemblyName(assemblyId))
		{
			std::wstring typeName = GetTypeAndMethodName(functionId);

			if (TARGETMETHOD2 == typeName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeName.c_str()));

				LPCBYTE pMethodHeader = NULL;
				ULONG iMethodSize = 0;
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBody(moduleId, functionToken, &pMethodHeader, &iMethodSize),
					_T("    ::JITCompilationStarted(...) => GetILFunctionBody => 0x%X"));

				IMAGE_COR_ILMETHOD* pMethod = (IMAGE_COR_ILMETHOD*)pMethodHeader;

				Method instumentedMethod(pMethod);
				InstructionList instructions; // NOTE: this IL will be different for an instance method or if the local vars signature is different
				instructions.push_back(new Instruction(CEE_LDARG, 0));
				instructions.push_back(new Instruction(CEE_CALL, m_targetPretendWeLoadedFakesProfilerRef));

				instumentedMethod.InsertInstructionsAtOriginalOffset(0, instructions);

				//instumentedMethod.DumpIL();

				// now to write the method back
				CComPtr<IMethodMalloc> methodMalloc;
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBodyAllocator(moduleId, &methodMalloc),
					_T("    ::JITCompilationStarted(...) => GetILFunctionBodyAllocator=> 0x%X"));
				IMAGE_COR_ILMETHOD* pNewMethod = (IMAGE_COR_ILMETHOD*)methodMalloc->Alloc(instumentedMethod.GetMethodSize());
				instumentedMethod.WriteMethod(pNewMethod);
				COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->SetILFunctionBody(moduleId, functionToken, (LPCBYTE)pNewMethod),
					_T("    ::JITCompilationStarted(...) => SetILFunctionBody => 0x%X"));
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