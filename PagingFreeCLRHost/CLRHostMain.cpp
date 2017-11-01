// PagingFreeCLRHost.cpp : Defines the entry point for the console application.
//
#include <metahost.h>
#include <wchar.h>
#include "CLRHostControl.h"

#pragma comment(lib, "mscoree.lib")

int wmain(int argc, wchar_t* argv[])
{
	wprintf(L"Non pageable CLR host");
	if (argc < 4)
	{
		wprintf(L"Usage: %S <Executable> [<EntryTypeName>][<EntryMethodName>][<Parameter - working set memory amount>]\n", argv[0]);
		wprintf(L"\nThe entry method must accept a single string and rerutn Int32.\n");
		return 0;
	}

	HRESULT hr;
	ICLRRuntimeHost* pCLR;

	hr = ::CorBindToRuntimeEx(L"v4.0.30319", L"svr", STARTUP_SERVER_GC|STARTUP_HOARD_GC_VM, CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&pCLR);
	if (FAILED(hr))
	{
		wprintf(L"Failed to load CLR: 0x%X\n", hr);
		ExitProcess(-1);
	}

	CLRHostControl hostControl(0.8);
	hr = pCLR->SetHostControl(&hostControl);
	if (FAILED(hr))
	{
		wprintf(L"Failed to set CLR host control: 0x%X\n", hr);
		ExitProcess(-1);
	}

	hr = pCLR->Start();
	if (FAILED(hr))
	{
		wprintf(L"Failed to start CLR: 0x%X\n", hr);
		ExitProcess(-1);
	}

	const wchar_t* application = argv[1];
	const wchar_t* entryType =  argv[2];
	const wchar_t* entryMethod = argv[3];
	const wchar_t* parameter = argc < 5 ? hostControl.EffectiveMinWorkingSetSizeString() : argv[4];

	DWORD returnVal;
	hr = pCLR->ExecuteInDefaultAppDomain(application, entryType, entryMethod, parameter, &returnVal);
	if (FAILED(hr))
		wprintf(L"Failed to execute assembly: 0x%X\n", hr);
	else
		wprintf(L"Assembly returned: %d\n", returnVal);

	hr = pCLR->Stop();
	if (FAILED(hr))
	{
		wprintf(L"Failed to stop CLR: 0x%X\n", hr);
		ExitProcess(-1);
	}

	ExitProcess(0);
	return 0;
}

