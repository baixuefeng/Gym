#include "UI/DirectX/D3DMisc.h"
#include <atlbase.h>
#pragma comment(lib, "dxgi.lib")

SHARELIB_BEGIN_NAMESPACE

bool GetVideoAdapterDescription(std::vector<DXGI_ADAPTER_DESC> &adapters)
{
    adapters.clear();
    ATL::CComPtr<IDXGIFactory> spDxgiFactory;
    HRESULT hr = ::CreateDXGIFactory(__uuidof(IDXGIFactory), (void **)&spDxgiFactory);
    UINT i = 0;
    while (SUCCEEDED(hr)) {
        ATL::CComPtr<IDXGIAdapter> spAdapter;
        hr = spDxgiFactory->EnumAdapters(i++, &spAdapter);
        if (FAILED(hr) && (hr == DXGI_ERROR_NOT_FOUND)) {
            return true;
        }
        DXGI_ADAPTER_DESC adapter{};
        hr = spAdapter->GetDesc(&adapter);
        if (FAILED(hr)) {
            return false;
        }
        adapters.push_back(adapter);
    }
    return false;
}

SHARELIB_END_NAMESPACE
