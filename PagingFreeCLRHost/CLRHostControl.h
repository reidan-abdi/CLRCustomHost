#include <mscoree.h>
#include <wchar.h>

class CLRHostControl : public IHostControl, public IHostMemoryManager
{
	static const int WorkingSizeAcquireDecrement = 64 * 1024 * 1024;
	DOUBLE _physMemoryPercentage;
	LONG _refCount;
	ICLRMemoryNotificationCallback* _pMemoryCallback;
	SIZE_T _effectiveMinWorkSetSize;
	typedef BOOL (WINAPI *PFN_SetProcessWorkingSetSize)(HANDLE, SIZE_T, SIZE_T, DWORD);

public:
	CLRHostControl(DOUBLE physMemPercentage);

	SIZE_T EffectiveMinWorkingSetSize() const;
	LPCWSTR EffectiveMinWorkingSetSizeString() const;

	//IUnknown
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppObject);

	//IHostControl
	HRESULT __stdcall GetHostManager(REFIID riid, void** ppObject);
	HRESULT __stdcall SetAppDomainManager(DWORD appDomainId, IUnknown* pUnkAppDomaintManager);

	//IHostMemoryManager
	HRESULT __stdcall AcquiredVirtualAddressSpace(LPVOID startAddress, SIZE_T size);
	HRESULT __stdcall ReleasedVirtualAddressSpace(LPVOID startAddress);
	HRESULT __stdcall NeedsVirtualAddressSpace(LPVOID startAddress, SIZE_T size);
	HRESULT __stdcall RegisterMemoryNotificationCallback(ICLRMemoryNotificationCallback* pCallback);
	HRESULT __stdcall GetMemoryLoad(DWORD* pMemoryLoad, SIZE_T* pAvailableBytes);
	HRESULT __stdcall VirtualAlloc(void* pAddress, SIZE_T size, DWORD flAllocationType, DWORD flProtect, EMemoryCriticalLevel criticalLevel, void** ppMem);
	HRESULT __stdcall VirtualFree(LPVOID lpAddress, SIZE_T size, DWORD freeType);
	HRESULT __stdcall VirtualProtect(void* lpAddress, SIZE_T size, DWORD newProtect, DWORD* pfOldProtect);
	HRESULT __stdcall VirtualQuery(void* lpAddress, void* lpBuffer, SIZE_T length, SIZE_T* pREsult);
	HRESULT __stdcall CreateMalloc(DWORD mallocType, IHostMalloc** ppMalloc);
};