#include "CLRHostMalloc.h"

CLRHostMalloc::CLRHostMalloc(MALLOC_TYPE type)
{
	_allocFlags = 0;

	DWORD heapFlags = 0;
	if (type & MALLOC_EXECUTABLE)
	{
		heapFlags |= HEAP_CREATE_ENABLE_EXECUTE;
	}

	if ((type & MALLOC_THREADSAFE) == 0)
	{
		heapFlags |= HEAP_NO_SERIALIZE;
		_allocFlags |= HEAP_NO_SERIALIZE;
	}

	_hMallocHeap = ::HeapCreate(heapFlags, 0, 0);
}

ULONG __stdcall CLRHostMalloc::AddRef()
{
	return InterlockedIncrement(&_refCount);
}

ULONG __stdcall CLRHostMalloc::Release()
{
	ULONG cRef = InterlockedDecrement(&_refCount);
	if (cRef == 0)
		delete this;

	return cRef;
}

HRESULT __stdcall CLRHostMalloc::QueryInterface(REFIID riid, void** ppObject)
{
	if (riid == IID_IUnknown)
	{
		*ppObject = static_cast<IUnknown*>(this);
		return S_OK;
	}
	if (riid == IID_IHostMalloc)
	{
		*ppObject = static_cast<IHostMalloc*>(this);
		return S_OK;
	}

	*ppObject = NULL;
	return E_NOINTERFACE;
}

HRESULT __stdcall CLRHostMalloc::Alloc(SIZE_T size, EMemoryCriticalLevel criticalLevel, void** ppMem)
{
	*ppMem = ::HeapAlloc(_hMallocHeap, _allocFlags, size);
	if (*ppMem == NULL)
		return E_OUTOFMEMORY;
	return S_OK;
}

HRESULT __stdcall CLRHostMalloc::DebugAlloc(SIZE_T size, EMemoryCriticalLevel criticalLevel, char* psFileName, int iLineNo, void** ppMem)
{
	wprintf(L"IHostMalloc::DebugAlloc called from file %s line %d\n", psFileName, iLineNo);
	return Alloc(size, criticalLevel, ppMem);
}

HRESULT __stdcall CLRHostMalloc::Free(void* pMem)
{
	if (::HeapFree(_hMallocHeap, 0, pMem) == FALSE)
		return E_FAIL;

	return S_OK;
}