#include "framework.h"
#include "DxApp.h"
#include <assert.h>

void DxApp::SetClearColor(float r, float g, float b, float a)
{
    m_ClearColor[0] = r;
    m_ClearColor[1] = g;
    m_ClearColor[2] = b;
    m_ClearColor[3] = a;
}

bool DxApp::Init(HWND hWnd, UINT width, UINT height)
{
    m_hWnd = hWnd;
    m_Width = (width == 0 ? 1u : width);
    m_Height = (height == 0 ? 1u : height);

    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr) || !pFactory) return false;

    IDXGIAdapter* pSelectedAdapter = nullptr;
    for (UINT i = 0;; ++i)
    {
        IDXGIAdapter* pAdapter = nullptr;
        hr = pFactory->EnumAdapters(i, &pAdapter);
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(hr) || !pAdapter) continue;

        DXGI_ADAPTER_DESC desc{};
        pAdapter->GetDesc(&desc);

        if (wcscmp(desc.Description, L"Microsoft Basic Render Driver") != 0)
        {
            pSelectedAdapter = pAdapter;
            break;
        }

        SAFE_RELEASE(pAdapter);
    }

    if (!pSelectedAdapter)
    {
        SAFE_RELEASE(pFactory);
        return false;
    }

    UINT deviceFlags = 0;
#if defined(_DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;

    hr = D3D11CreateDevice(
        pSelectedAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        deviceFlags,
        levels, 1,
        D3D11_SDK_VERSION,
        &m_pDevice,
        &level,
        &m_pDeviceContext
    );

    SAFE_RELEASE(pSelectedAdapter);

    if (FAILED(hr) || !m_pDevice || !m_pDeviceContext) { SAFE_RELEASE(pFactory); return false; }
    if (level != D3D_FEATURE_LEVEL_11_0) { SAFE_RELEASE(pFactory); return false; }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferDesc.Width = m_Width;
    swapChainDesc.BufferDesc.Height = m_Height;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;                 
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;             
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; 
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;                

    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = m_hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; 
    swapChainDesc.Flags = 0;

    hr = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
    SAFE_RELEASE(pFactory);
    if (FAILED(hr) || !m_pSwapChain) return false;

    if (!CreateBackBufferRTV()) return false;

    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);

    return true;
}

void DxApp::Shutdown()
{
    ReleaseBackBufferRTV();

    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);

#if defined(_DEBUG)
    if (m_pDevice)
    {
        ID3D11Debug* debug = nullptr;
        m_pDevice->QueryInterface(IID_PPV_ARGS(&debug));

        UINT references = m_pDevice->Release();
        if (debug && references > 1)
            debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

        SAFE_RELEASE(debug);
        m_pDevice = nullptr;
    }
#else
    SAFE_RELEASE(m_pDevice);
#endif
}

bool DxApp::CreateBackBufferRTV()
{
    if (!m_pSwapChain || !m_pDevice) return false;

    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || !backBuffer) return false;

    hr = m_pDevice->CreateRenderTargetView(backBuffer, nullptr, &m_pBackBufferRTV);
    SAFE_RELEASE(backBuffer);

    return SUCCEEDED(hr) && m_pBackBufferRTV != nullptr;
}

void DxApp::ReleaseBackBufferRTV()
{
    if (m_pDeviceContext)
        m_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    SAFE_RELEASE(m_pBackBufferRTV);
}

void DxApp::OnResize(UINT width, UINT height)
{
    if (!m_pSwapChain || !m_pDeviceContext) return;
    if (width == 0 || height == 0) return;

    m_Width = width;
    m_Height = height;

    ReleaseBackBufferRTV();

    HRESULT hr = m_pSwapChain->ResizeBuffers(
        2,
        m_Width,
        m_Height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0
    );
    if (FAILED(hr)) return;

    if (!CreateBackBufferRTV()) return;

    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);
}


void DxApp::Render()
{
    if (!m_pDeviceContext || !m_pBackBufferRTV || !m_pSwapChain) return;

    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };

    m_pDeviceContext->ClearState();
    m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);
    m_pDeviceContext->ClearRenderTargetView(views[0], m_ClearColor);

    HRESULT hr = m_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(hr));
}
