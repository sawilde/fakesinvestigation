// ProfilerHook.cpp : Implementation of CProfilerHook

#include "stdafx.h"
#include "ProfilerHook.h"
#include "Method.h"
#include "ProfilerInfo.h"

#define TESTPLATFORM_UTILITIES_ASSEMBLY L"Microsoft.VisualStudio.TestPlatform.Utilities"
#define DEFAULTTESTEXECUTOR_LAUNCHPROCESS L"Microsoft.VisualStudio.TestPlatform.Utilities.DefaultTestExecutorLauncher::LaunchProcess"
#define DEFAULTTESTEXECUTOR_CTOR L"Microsoft.VisualStudio.TestPlatform.Utilities.DefaultTestExecutorLauncher::.ctor"

#define TESTPLATFORM_TESTEXECUTOR_CORE_ASSEMBLY L"Microsoft.VisualStudio.TestPlatform.TestExecutor.Core"
#define TESTEXECUTORMAIN_RUN L"Microsoft.VisualStudio.TestPlatform.TestExecutor.TestExecutorMain::Run"
#define TESTEXECUTORMAIN_CTOR L"Microsoft.VisualStudio.TestPlatform.TestExecutor.TestExecutorMain::.ctor"

#import <mscorlib.tlb> raw_interfaces_only
using namespace mscorlib;

namespace {
	struct __declspec(uuid("2180EC45-CF11-456E-9A76-389A4521A4BE"))
	IFakesDomainHelper : IUnknown
	{
		virtual HRESULT __stdcall AddResolveEventHandler() = 0;
	};
}

LPSAFEARRAY GetInjectedDllAsSafeArray()
{
	HRSRC hClrHookDllRes = FindResource(g_hInst, MAKEINTRESOURCE(IDR_INJECTED), L"BINDATA");
	ATLASSERT(hClrHookDllRes != NULL);

	HGLOBAL hClrHookDllHGlb = LoadResource(g_hInst, hClrHookDllRes);
	ATLASSERT(hClrHookDllHGlb != NULL);

	DWORD dllMemorySize = SizeofResource(g_hInst, hClrHookDllRes);

	LPBYTE lpDllData = (LPBYTE)LockResource(hClrHookDllHGlb);
	ATLASSERT(lpDllData != NULL);

	SAFEARRAYBOUND bound = { 0 };
	bound.cElements = dllMemorySize;
	bound.lLbound = 0;

	LPBYTE lpArrayData;
	LPSAFEARRAY lpAsmblyData = SafeArrayCreate(VT_UI1, 1, &bound);
	ATLASSERT(lpAsmblyData != NULL);

	SafeArrayAccessData(lpAsmblyData, (void**)&lpArrayData);
	memcpy(lpArrayData, lpDllData, dllMemorySize);
	SafeArrayUnaccessData(lpAsmblyData);

	return lpAsmblyData;
}


EXTERN_C HRESULT STDAPICALLTYPE LoadInjectorAssembly(IUnknown *pUnk)
{
	ATLTRACE(_T("****LoadInjectorAssembly - Start****"));

	CComPtr<_AppDomain> pAppDomain;
	HRESULT hr = pUnk->QueryInterface(__uuidof(_AppDomain), (void**)&pAppDomain);
	ATLASSERT(hr == S_OK);
	LPSAFEARRAY lpAsmblyData = GetInjectedDllAsSafeArray();
	ATLASSERT(lpAsmblyData != NULL);

	CComPtr<_Assembly> pAssembly;
	hr = pAppDomain->Load_3(lpAsmblyData, &pAssembly);
	ATLASSERT(hr==S_OK);

	SafeArrayDestroy(lpAsmblyData);

	CComVariant variant;
	hr = pAssembly->CreateInstance(W2BSTR(L"Injected.FakesDomainHelper"), &variant);
	ATLASSERT(hr == S_OK);

	CComPtr<IFakesDomainHelper> pFakesDomainHelper;
	hr = variant.punkVal->QueryInterface(__uuidof(IFakesDomainHelper), (void**)&pFakesDomainHelper);
	ATLASSERT(hr == S_OK);

	hr = pFakesDomainHelper->AddResolveEventHandler();
	ATLASSERT(hr == S_OK);
	ATLTRACE(_T("****LoadInjectorAssembly - End****"));

	return S_OK;
}

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

mdSignature CProfilerHook::GetMethodSignatureToken_OBJ(ModuleID moduleID)
{
	static COR_SIGNATURE unmanagedCallSignature[] =
	{
		IMAGE_CEE_CS_CALLCONV_DEFAULT,          // Default CallKind!
		0x01,                                   // Parameter count
		ELEMENT_TYPE_VOID,                      // Return type
		ELEMENT_TYPE_OBJECT                     // Parameter type (OBJECT)
	};

	CComPtr<IMetaDataEmit> metaDataEmit;
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit), 0,
		_T("    ::GetMethodSignatureToken_OBJ(ModuleID) => GetModuleMetaData => 0x%X"));

	mdSignature pmsig;
	COM_FAIL_MSG_RETURN_OTHER(metaDataEmit->GetTokenFromSig(unmanagedCallSignature, sizeof(unmanagedCallSignature), &pmsig), 0,
		_T("    ::GetMethodSignatureToken_OBJ(ModuleID) => GetTokenFromSig => 0x%X"));
	return pmsig;
}

mdMethodDef CProfilerHook::Get_CurrentDomainMethod(ModuleID moduleID)
{
	CComPtr<IMetaDataEmit> metaDataEmit;
	COM_FAIL_MSG_RETURN_OTHER(m_profilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit), 0,
		_T("    ::Get_CurrentDomainMethod(ModuleID) => GetModuleMetaData => 0x%X"));

	mdModuleRef mscorlibRef;
	COM_FAIL_RETURN(GetMsCorlibRef(moduleID, mscorlibRef), S_OK);
	
	mdMethodDef getCurrentDomain;
	mdTypeDef tkAppDomain;
	metaDataEmit->DefineTypeRefByName(mscorlibRef, L"System.AppDomain", &tkAppDomain);
	BYTE importSig[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT, 0 /*<no args*/, 0x12 /*< ret class*/, 0, 0, 0, 0, 0 };
	ULONG l = CorSigCompressToken(tkAppDomain, importSig + 3);	//< Add the System.AppDomain token ref
	metaDataEmit->DefineMemberRef(tkAppDomain, L"get_CurrentDomain", importSig, 3 + l, &getCurrentDomain);
	return getCurrentDomain;
}

HRESULT STDMETHODCALLTYPE CProfilerHook::ModuleAttachedToAssembly(
	/* [in] */ ModuleID moduleId,
	/* [in] */ AssemblyID assemblyId)
{
	if (m_chainedProfiler != NULL)
		m_chainedProfiler->ModuleAttachedToAssembly(moduleId, assemblyId);

	std::wstring assemblyName = GetAssemblyName(assemblyId);

	if (TESTPLATFORM_UTILITIES_ASSEMBLY == GetAssemblyName(assemblyId))
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

		m_pinvokeAttach = CreatePInvokeHook(metaDataEmit);
	}

	if (TESTPLATFORM_TESTEXECUTOR_CORE_ASSEMBLY == GetAssemblyName(assemblyId))
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

		m_pinvokeAttach = CreatePInvokeHook(metaDataEmit);
		
	}

	return S_OK;
}

mdMethodDef CProfilerHook::CreatePInvokeHook(IMetaDataEmit* pMetaDataEmit){
	
	mdTypeDef	tkInjClass;

	HRESULT hr = pMetaDataEmit->DefineTypeDef(L"__ClrProbeInjection_", tdAbstract | tdSealed, m_objectTypeRef, NULL, &tkInjClass);
	ATLASSERT(hr == S_OK);

	static BYTE sg_sigPLoadInjectorAssembly[] = {
		0, // IMAGE_CEE_CS_CALLCONV_DEFAULT
		1, // argument count
		ELEMENT_TYPE_VOID, // ret = ELEMENT_TYPE_VOID
		ELEMENT_TYPE_OBJECT
	};

	mdModuleRef	tkRefClrProbe;
	hr = pMetaDataEmit->DefineModuleRef(L"SPIKEPROFILER.DLL", &tkRefClrProbe);
	ATLASSERT(hr == S_OK);

	mdMethodDef	tkAttachDomain;
	pMetaDataEmit->DefineMethod(tkInjClass, L"LoadInjectorAssembly",
		mdStatic | mdPinvokeImpl,
		sg_sigPLoadInjectorAssembly, sizeof(sg_sigPLoadInjectorAssembly),
		0, 0, &tkAttachDomain
		);
	ATLASSERT(hr == S_OK);

	BYTE tiunk = NATIVE_TYPE_IUNKNOWN;
	mdParamDef paramDef;
	hr = pMetaDataEmit->DefinePinvokeMap(tkAttachDomain, 0, L"LoadInjectorAssembly", tkRefClrProbe);
	ATLASSERT(hr == S_OK);

	hr = pMetaDataEmit->DefineParam(tkAttachDomain, 1, L"appDomain",
		pdIn | pdHasFieldMarshal, 0, NULL, 0, &paramDef);
	ATLASSERT(hr == S_OK);
	
	hr = pMetaDataEmit->SetFieldMarshal(paramDef, &tiunk, 1);
	ATLASSERT(hr == S_OK);

	return tkAttachDomain;
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

		if (TESTPLATFORM_UTILITIES_ASSEMBLY == GetAssemblyName(assemblyId))
		{
			std::wstring typeMethodName = GetTypeAndMethodName(functionId);

			if (DEFAULTTESTEXECUTOR_CTOR == typeMethodName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeMethodName.c_str()));

				InstructionList instructions; // NOTE: this IL will be different for an instance method or if the local vars signature is different

				mdMethodDef getCurrentDomain = Get_CurrentDomainMethod(moduleId);
				instructions.push_back(new Instruction(CEE_CALL, getCurrentDomain));
				instructions.push_back(new Instruction(CEE_CALL, m_pinvokeAttach));

				InstrumentMethodWith(moduleId, functionToken, instructions);
			}

			if (DEFAULTTESTEXECUTOR_LAUNCHPROCESS == typeMethodName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeMethodName.c_str()));

				InstructionList instructions; // NOTE: this IL will be different for an instance method or if the local vars signature is different

				instructions.push_back(new Instruction(CEE_LDARG_S, 4));
				instructions.push_back(new Instruction(CEE_CALL, m_targetLoadProfilerInsteadRef));

				InstrumentMethodWith(moduleId, functionToken, instructions);
			}

			return S_OK;
		}

		if (TESTPLATFORM_TESTEXECUTOR_CORE_ASSEMBLY == GetAssemblyName(assemblyId))
		{
			std::wstring typeMethodName = GetTypeAndMethodName(functionId);

			if (TESTEXECUTORMAIN_CTOR == typeMethodName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeMethodName.c_str()));

				InstructionList instructions; 

				mdMethodDef getCurrentDomain = Get_CurrentDomainMethod(moduleId);
				instructions.push_back(new Instruction(CEE_CALL, getCurrentDomain));
				instructions.push_back(new Instruction(CEE_CALL, m_pinvokeAttach));

				InstrumentMethodWith(moduleId, functionToken, instructions);

			}

			if (TESTEXECUTORMAIN_RUN == typeMethodName)
			{
				ATLTRACE(_T("::JITCompilationStarted(%X, ...) => %d, %X => %s"), functionId, functionToken, moduleId, W2CT(typeMethodName.c_str()));

				InstructionList instructions; // NOTE: this IL will be different for an instance method or if the local vars signature is different

				instructions.push_back(new Instruction(CEE_LDARG, 0));
				instructions.push_back(new Instruction(CEE_CALL, m_targetPretendWeLoadedFakesProfilerRef));

				InstrumentMethodWith(moduleId, functionToken, instructions);
			}
		}
	}

	return S_OK;
}

HRESULT CProfilerHook::InstrumentMethodWith(ModuleID moduleId, mdToken functionToken, InstructionList &instructions){

	LPCBYTE pMethodHeader = NULL;
	ULONG iMethodSize = 0;
	COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBody(moduleId, functionToken, &pMethodHeader, &iMethodSize),
		_T("    ::JITCompilationStarted(...) => GetILFunctionBody => 0x%X"));

	IMAGE_COR_ILMETHOD* pMethod = (IMAGE_COR_ILMETHOD*)pMethodHeader;
	Method instumentedMethod(pMethod);

	instumentedMethod.InsertInstructionsAtOriginalOffset(0, instructions);

	instumentedMethod.DumpIL();

	// now to write the method back
	CComPtr<IMethodMalloc> methodMalloc;
	COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetILFunctionBodyAllocator(moduleId, &methodMalloc),
		_T("    ::JITCompilationStarted(...) => GetILFunctionBodyAllocator=> 0x%X"));
	IMAGE_COR_ILMETHOD* pNewMethod = (IMAGE_COR_ILMETHOD*)methodMalloc->Alloc(instumentedMethod.GetMethodSize());
	instumentedMethod.WriteMethod(pNewMethod);
	COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->SetILFunctionBody(moduleId, functionToken, (LPCBYTE)pNewMethod),
		_T("    ::JITCompilationStarted(...) => SetILFunctionBody => 0x%X"));
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

HRESULT CProfilerHook::GetModuleRef(ModuleID moduleId, WCHAR*moduleName, mdModuleRef &mscorlibRef)
{
	CComPtr<IMetaDataEmit> metaDataEmit;
	COM_FAIL_MSG_RETURN_ERROR(m_profilerInfo->GetModuleMetaData(moduleId,
		ofRead | ofWrite, IID_IMetaDataEmit, (IUnknown**)&metaDataEmit),
		_T("GetModuleRef(...) => GetModuleMetaData => 0x%X"));

	CComPtr<IMetaDataAssemblyEmit> metaDataAssemblyEmit;
	COM_FAIL_MSG_RETURN_ERROR(metaDataEmit->QueryInterface(
		IID_IMetaDataAssemblyEmit, (void**)&metaDataAssemblyEmit),
		_T("GetModuleRef(...) => QueryInterface => 0x%X"));

	return GetModuleRef4000(metaDataAssemblyEmit, moduleName, mscorlibRef);
}

HRESULT CProfilerHook::GetModuleRef4000(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef)
{
	ASSEMBLYMETADATA assembly;
	ZeroMemory(&assembly, sizeof(assembly));
	assembly.usMajorVersion = 4;
	assembly.usMinorVersion = 0;
	assembly.usBuildNumber = 0;
	assembly.usRevisionNumber = 0;
	BYTE publicKey[] = { 0xB7, 0x7A, 0x5C, 0x56, 0x19, 0x34, 0xE0, 0x89 };
	COM_FAIL_MSG_RETURN_ERROR(metaDataAssemblyEmit->DefineAssemblyRef(publicKey,
		sizeof(publicKey), moduleName, &assembly, NULL, 0, 0,
		&mscorlibRef), _T("GetModuleRef4000(...) => DefineAssemblyRef => 0x%X"));

	return S_OK;
}

HRESULT CProfilerHook::GetModuleRef2000(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef)
{
	ASSEMBLYMETADATA assembly;
	ZeroMemory(&assembly, sizeof(assembly));
	assembly.usMajorVersion = 2;
	assembly.usMinorVersion = 0;
	assembly.usBuildNumber = 0;
	assembly.usRevisionNumber = 0;
	BYTE publicKey[] = { 0xB7, 0x7A, 0x5C, 0x56, 0x19, 0x34, 0xE0, 0x89 };
	COM_FAIL_MSG_RETURN_ERROR(metaDataAssemblyEmit->DefineAssemblyRef(publicKey,
		sizeof(publicKey), moduleName, &assembly, NULL, 0, 0, &mscorlibRef),
		_T("GetModuleRef2000(...) => DefineAssemblyRef => 0x%X"));

	return S_OK;
}

HRESULT CProfilerHook::GetModuleRef2050(IMetaDataAssemblyEmit *metaDataAssemblyEmit, WCHAR*moduleName, mdModuleRef &mscorlibRef)
{
	ASSEMBLYMETADATA assembly;
	ZeroMemory(&assembly, sizeof(assembly));
	assembly.usMajorVersion = 2;
	assembly.usMinorVersion = 0;
	assembly.usBuildNumber = 5;
	assembly.usRevisionNumber = 0;

	BYTE publicKey[] = { 0x7C, 0xEC, 0x85, 0xD7, 0xBE, 0xA7, 0x79, 0x8E };
	COM_FAIL_MSG_RETURN_ERROR(metaDataAssemblyEmit->DefineAssemblyRef(publicKey,
		sizeof(publicKey), moduleName, &assembly, NULL, 0, 0, &mscorlibRef),
		_T("GetModuleRef2050(...) => DefineAssemblyRef => 0x%X"));

	return S_OK;
}

