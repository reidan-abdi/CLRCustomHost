#include <metahost.h>
#include <wchar.h>

class CLRHostMalloc : public IHostMalloc
{
	LONG _refCount;
	HANDLE _hMallocHeap;
	DWORD _allocFlags;

public:
	CLRHostMalloc(MALLOC_TYPE type);

	//IUnknown
	ULONG __stdcall AddRef();
	ULONG __stdcall Release();
	HRESULT __stdcall QueryInterface(REFIID riid, void** ppObject);
	
	//IHostMalloc
	HRESULT __stdcall Alloc(SIZE_T size, EMemoryCriticalLevel criticalLevel, void** ppMem);
	HRESULT __stdcall DebugAlloc(SIZE_T size, EMemoryCriticalLevel criticalLevel, char* pFileName, int iLineNo, void** ppMem);
	HRESULT __stdcall Free(void* pMem);
};