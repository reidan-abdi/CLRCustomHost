#include "CLRHostControl.h"
#include "CLRHostMalloc.h"

SIZE_T CLRHostControl::EffectiveMinWorkingSetSize() const
{
	return _effectiveMinWorkSetSize;
}

LPCWSTR CLRHostControl::EffectiveMinWorkingSetSizeString() const
{
	wchar_t sMinWsSize[200];
	wsprintf(sMinWsSize, L"%Iu", EffectiveMinWorkingSetSize());
	return sMinWsSize;
}

CLRHostControl::CLRHostControl(DOUBLE physMemoryPercentage)
{
	_refCount = 0;
	_pMemoryCallback = NULL;
	_effectiveMinWorkSetSize = 0;
	_physMemoryPercentage = physMemoryPercentage;

	MEMORYSTATUSEX memStatus;
	if (!GlobalMemoryStatusEx(&memStatus))
	{
		wprintf(L"GlobalMemoryStatusEx failed: %d\n", GetLastError());
		ExitProcess(1);
	}

#if defined(_WIN64)
	SIZE_T sizeToAcquire = min(memStatus.ullTotalPhys * _physMemoryPercentage, memStatus.ullTotalVirtual);
#else
#pragma warning(push)
#pragma warning(disable:4244)
	SIZE_T sizeToAcquire = min(0xFFFFFFFF, memStatus.ullTotalPhys);
#pragma warning (pop)
#endif
	
	if (_physMemoryPercentage > 0.95)
		sizeToAcquire -= 128 * 1024 * 1024;
	
	DWORD dwQuotaFlags = QUOTA_LIMITS_HARDWS_MIN_ENABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
	while (!SetProcessWorkingSetSizeEx(::GetCurrentProcess(), sizeToAcquire, sizeToAcquire, dwQuotaFlags))
	{
		if (sizeToAcquire <= WorkingSizeAcquireDecrement)
		{
			wprintf(L"Failed to set working set size limits for host.\n");
			break;
		}

		if (GetLastError() == ERROR_INVALID_PARAMETER)
		{
			dwQuotaFlags = QUOTA_LIMITS_HARDWS_MIN_DISABLE | QUOTA_LIMITS_HARDWS_MAX_DISABLE;
			continue;
		}

		if (GetLastError() == ERROR_NO_SYSTEM_RESOURCES)
		{
			sizeToAcquire -= WorkingSizeAcquireDecrement;
			continue;
		}

		wprintf(L"Failed to call SetProcessWorkingSetSizeEx to %Iu bytes: %d\n", sizeToAcquire, GetLastError());
		break;
	}

	wprintf(L"Successfully called SetProcessWorkingSetSizeEx to %Iu bytes\n", sizeToAcquire);
	_effectiveMinWorkSetSize = sizeToAcquire;
}

ULONG __stdcall CLRHostControl::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

ULONG __stdcall CLRHostControl::Release()
{
	return InterlockedDecrement(&_refCount);
}

HRESULT __stdcall CLRHostControl::QueryInterface(REFIID riid, void** ppObject)
{
	if (riid == IID_IUnknown)
	{
		*ppObject = static_cast<IUnknown*>(static_cast<IHostControl*>(this));
		return S_OK;
	}
	if (riid == IID_IHostControl)
	{
		*ppObject = static_cast<IHostControl*>(this);
		return S_OK;
	}
	if (riid == IID_IHostMemoryManager)
	{
		*ppObject = static_cast<IHostMemoryManager*>(this);
		return S_OK;
	}

	*ppObject = NULL;
	return E_NOINTERFACE;
}

HRESULT __stdcall CLRHostControl::GetHostManager(REFIID riid, void** ppObject)
{
	if (riid == IID_IHostMemoryManager)
	{
		*ppObject = static_cast<IHostMemoryManager*>(this);
		return S_OK;
	}

	*ppObject = NULL;
	return E_NOINTERFACE;
}

HRESULT __stdcall CLRHostControl::SetAppDomainManager(DWORD appDomainId, IUnknown* pUnkAppDomainManager)
{
	return S_OK;
}

HRESULT __stdcall CLRHostControl::AcquiredVirtualAddressSpace(LPVOID startAddress, SIZE_T size)
{
	wprintf(L"CLRHostControl::AcquiredVirtualAddressSpace: Acquired %Iu bytes from address 0x%IX\n", size, startAddress);

	return S_OK;
}

HRESULT __stdcall CLRHostControl::ReleasedVirtualAddressSpace(LPVOID startAddress)
{
	wprintf(L"CLRHostControl::ReleasedVirtualAddressSpace: Released address 0x%IX\n", startAddress);

	return S_OK;
}

HRESULT __stdcall CLRHostControl::NeedsVirtualAddressSpace(LPVOID startAddress, SIZE_T size)
{
	wprintf(L"CLRHostControl::NeedsVirtualAddressSpace: Going to use %Iu bytes from address 0x%IX\n", size, startAddress);

	return S_OK;
}

HRESULT __stdcall CLRHostControl::RegisterMemoryNotificationCallback(ICLRMemoryNotificationCallback* pCallback)
{
	_pMemoryCallback = pCallback;
	return S_OK;
}

HRESULT __stdcall CLRHostControl::GetMemoryLoad(DWORD* pMemoryLoad, SIZE_T* pAvailableBytes)
{
	MEMORYSTATUSEX memStatus;
	memStatus.dwLength = sizeof(memStatus);
	if (!GlobalMemoryStatusEx(&memStatus))
	{
		wprintf(L"GetMemoryLoad error - %d\n", GetLastError());
		return E_FAIL;
	}

	*pMemoryLoad = memStatus.dwMemoryLoad;
#if defined(_WIN64)
	*pAvailableBytes = memStatus.ullAvailPhys;
#else
#pragma warning (push)
#pragma warning (disable:4244)
	*pAvailableBytes = min(0xFFFFFFFF, memStatus.ullAvailPhys);
#pragma warning (pop)
#endif

	wprintf(L"CLRHostControl::GetMemoryLoad: Memory - %d and %Iu available bytes\n", *pMemoryLoad, *pAvailableBytes);
	return S_OK;
}

HRESULT __stdcall CLRHostControl::VirtualAlloc(void* pAddress, SIZE_T size, DWORD flallocationType, DWORD flProtect, EMemoryCriticalLevel criticalLevel, void** ppMem)
{
	*ppMem = ::VirtualAlloc(pAddress, size, flallocationType, flProtect);
	if (*ppMem == NULL)
	{
		wprintf(L"CLRHostControl::VirtualAllocL Failed to allocate %Iu bytes at 0x%IX\n", size, pAddress);
		return E_OUTOFMEMORY;
	}

	if (flallocationType & MEM_COMMIT)
	{
		if (::VirtualLock(*ppMem, size))
			wprintf(L"CLRHostControl::VirtualAllocL Successfully locked %Iu bytes at 0x%IX\n", size, pAddress);
		else
			wprintf(L"CLRHostControl::VirtualAllocL Failed to lock %Iu bytes at 0x%IX\n", size, pAddress);
	}

	wprintf(L"CLRHostControl::VirtualAllocL Successfully allocated %Iu bytes at 0x%IX\n", size, pAddress);
	return S_OK;
}

HRESULT __stdcall CLRHostControl::VirtualFree(LPVOID lpAddress, SIZE_T size, DWORD freeType)
{
	if (::VirtualUnlock(lpAddress, size))
		wprintf(L"CLRHostControl::VirtualFree Successfully unlocked %Iu bytes in memory\n", size);
	else
		wprintf(L"CLRHostControl::VirtualFree Failed to unlock %Iu bytes in memory\n", size);

	BOOL res = ::VirtualFree(lpAddress, size, freeType);
	if (!res)
	{
		wprintf(L"CLRHostControl::VirtualFree Failed to free %Iu bytes at 0x%IX\n", size, lpAddress);
		return E_FAIL;
	}

	wprintf(L"CLRHostControl::VirtualFree Successfully freed %Iu bytes at 0x%IX\n", size, lpAddress);
	return S_OK;
}

HRESULT __stdcall CLRHostControl::VirtualProtect(void* lpAddress, SIZE_T size, DWORD newProtect, DWORD* pOldProtect)
{
	BOOL res = ::VirtualProtect(lpAddress, size, newProtect, pOldProtect);
	if (!res)
	{
		wprintf(L"CLRHostControl::VirtualProtect Failed to protect %Iu bytes at 0x%IX\n", size, lpAddress);
		return E_FAIL;
	}

	wprintf(L"CLRHostControl::VirtualProtect Successfully protected %Iu bytes at 0x%IX\n", size, lpAddress);
	return S_OK;
}

HRESULT __stdcall CLRHostControl::VirtualQuery(void* lpAddress, void* lpBuffer, SIZE_T length, SIZE_T* pResult)
{
	*pResult = ::VirtualQuery(lpAddress, (PMEMORY_BASIC_INFORMATION)lpBuffer, length);
	wprintf(L"HostControl::VirtualQuery: Called for 0x%IX\n", lpAddress);
	return S_OK;
}

HRESULT __stdcall CLRHostControl::CreateMalloc(DWORD dwMallocType, IHostMalloc** ppMalloc)
{
	*ppMalloc = new CLRHostMalloc((MALLOC_TYPE)dwMallocType);
	return S_OK;
}

