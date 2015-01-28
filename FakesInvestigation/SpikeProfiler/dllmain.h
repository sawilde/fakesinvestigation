// dllmain.h : Declaration of module class.

class CSpikeProfilerModule : public ATL::CAtlDllModuleT< CSpikeProfilerModule >
{
public :
	DECLARE_LIBID(LIBID_SpikeProfilerLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SPIKEPROFILER, "{15FA5249-D5F2-453A-A8EF-F75DFFE960FD}")
};

extern class CSpikeProfilerModule _AtlModule;
