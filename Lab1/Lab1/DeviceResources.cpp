#include "framework.h"
#include "DeviceResources.h"

#include "ShaderUtils.h"

#include <assert.h>

bool DeviceResources::Init(HWND hWnd, UINT width, UINT height)
{
    m_hWnd = hWnd;
    m_Width = (width == 0 ? 1u : width);
    m_Height = (height == 0 ? 1u : height);

    if (!CreateDeviceAndSwapChain())
        return false;
    if (!CreateBackBufferRTV())
        return false;
    if (!CreateDepthBuffer())
        return false;
    if (!CreateColorBuffer())
        return false;

    return true;
}

bool DeviceResources::CreateDeviceAndSwapChain()
{
    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&pFactory));
    if (FAILED(hr) || !pFactory)
        return false;

    IDXGIAdapter* pSelectedAdapter = nullptr;
    for (UINT i = 0;; ++i)
    {
        IDXGIAdapter* pAdapter = nullptr;
        hr = pFactory->EnumAdapters(i, &pAdapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(hr) || !pAdapter)
            continue;

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

    UINT flags = 0;
#if defined(_DEBUG)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;

    hr = D3D11CreateDevice(
        pSelectedAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        nullptr,
        flags,
        levels,
        1,
        D3D11_SDK_VERSION,
        &m_pDevice,
        &level,
        &m_pDeviceContext);

    SAFE_RELEASE(pSelectedAdapter);

    if (FAILED(hr) || !m_pDevice || !m_pDeviceContext)
    {
        SAFE_RELEASE(pFactory);
        return false;
    }

    if (level != D3D_FEATURE_LEVEL_11_0)
    {
        SAFE_RELEASE(pFactory);
        return false;
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc{};
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = m_Width;
    swapChainDesc.BufferDesc.Height = m_Height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = m_hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;

    hr = pFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
    SAFE_RELEASE(pFactory);

    return SUCCEEDED(hr) && m_pSwapChain != nullptr;
}

bool DeviceResources::CreateBackBufferRTV()
{
    if (!m_pSwapChain || !m_pDevice)
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    const HRESULT hr = m_pSwapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pBackBuffer));

    if (FAILED(hr) || !pBackBuffer)
        return false;

    const HRESULT viewHr = m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pBackBufferRTV);
    SAFE_RELEASE(pBackBuffer);

    if (FAILED(viewHr) || !m_pBackBufferRTV)
        return false;

    SetResourceName(m_pBackBufferRTV, "BackBufferRTV");
    return true;
}

void DeviceResources::ReleaseBackBufferRTV()
{
    if (m_pDeviceContext)
        m_pDeviceContext->OMSetRenderTargets(0, nullptr, nullptr);

    SAFE_RELEASE(m_pBackBufferRTV);
}

bool DeviceResources::CreateDepthBuffer()
{
    if (!m_pDevice)
        return false;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = m_Width;
    desc.Height = m_Height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pDepthTexture);
    if (FAILED(hr) || !m_pDepthTexture)
        return false;

    hr = m_pDevice->CreateDepthStencilView(m_pDepthTexture, nullptr, &m_pDepthStencilView);
    if (FAILED(hr) || !m_pDepthStencilView)
    {
        SAFE_RELEASE(m_pDepthTexture);
        return false;
    }

    SetResourceName(m_pDepthTexture, "DepthBuffer");
    SetResourceName(m_pDepthStencilView, "DepthBufferDSV");
    return true;
}

void DeviceResources::ReleaseDepthBuffer()
{
    SAFE_RELEASE(m_pDepthStencilView);
    SAFE_RELEASE(m_pDepthTexture);
}

bool DeviceResources::CreateColorBuffer()
{
    if (!m_pDevice)
        return false;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.ArraySize = 1;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Height = m_Height;
    desc.Width = m_Width;
    desc.MipLevels = 1;

    HRESULT hr = m_pDevice->CreateTexture2D(&desc, nullptr, &m_pColorBuffer);
    if (FAILED(hr) || !m_pColorBuffer)
        return false;

    hr = m_pDevice->CreateRenderTargetView(m_pColorBuffer, nullptr, &m_pColorBufferRTV);
    if (FAILED(hr) || !m_pColorBufferRTV)
    {
        SAFE_RELEASE(m_pColorBuffer);
        return false;
    }

    hr = m_pDevice->CreateShaderResourceView(m_pColorBuffer, nullptr, &m_pColorBufferSRV);
    if (FAILED(hr) || !m_pColorBufferSRV)
    {
        SAFE_RELEASE(m_pColorBufferRTV);
        SAFE_RELEASE(m_pColorBuffer);
        return false;
    }

    SetResourceName(m_pColorBuffer, "ColorBuffer");
    SetResourceName(m_pColorBufferRTV, "ColorBufferRTV");
    SetResourceName(m_pColorBufferSRV, "ColorBufferSRV");

    return true;
}

void DeviceResources::ReleaseColorBuffer()
{
    SAFE_RELEASE(m_pColorBufferSRV);
    SAFE_RELEASE(m_pColorBufferRTV);
    SAFE_RELEASE(m_pColorBuffer);
}

void DeviceResources::OnResize(UINT width, UINT height)
{
    if (!m_pSwapChain || !m_pDeviceContext)
        return;
    if (width == 0 || height == 0)
        return;

    m_Width = width;
    m_Height = height;

    ReleaseColorBuffer();
    ReleaseBackBufferRTV();
    ReleaseDepthBuffer();

    const HRESULT hr = m_pSwapChain->ResizeBuffers(
        2,
        m_Width,
        m_Height,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        0);

    if (FAILED(hr))
        return;

    if (!CreateBackBufferRTV())
        return;
    if (!CreateDepthBuffer())
        return;
    if (!CreateColorBuffer())
        return;

    ID3D11RenderTargetView* views[] = { m_pColorBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, m_pDepthStencilView);
}

void DeviceResources::BeginFrame(const float clearColor[4])
{
    if (!IsReady())
        return;

    m_pDeviceContext->ClearState();

    ID3D11RenderTargetView* views[] = { m_pColorBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, m_pDepthStencilView);
    m_pDeviceContext->ClearRenderTargetView(m_pColorBufferRTV, clearColor);
    m_pDeviceContext->ClearDepthStencilView(
        m_pDepthStencilView,
        D3D11_CLEAR_DEPTH,
        0.0f,
        0);

    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(m_Width);
    viewport.Height = static_cast<FLOAT>(m_Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_pDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect{};
    rect.left = 0;
    rect.top = 0;
    rect.right = static_cast<LONG>(m_Width);
    rect.bottom = static_cast<LONG>(m_Height);
    m_pDeviceContext->RSSetScissorRects(1, &rect);
}

void DeviceResources::Present()
{
    if (!m_pSwapChain)
        return;

    const HRESULT hr = m_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(hr));
}

void DeviceResources::Shutdown()
{
    ReleaseColorBuffer();
    ReleaseDepthBuffer();
    ReleaseBackBufferRTV();

    SAFE_RELEASE(m_pSwapChain);
    SAFE_RELEASE(m_pDeviceContext);

#if defined(_DEBUG)
    if (m_pDevice)
    {
        ID3D11Debug* pDebug = nullptr;
        m_pDevice->QueryInterface(IID_PPV_ARGS(&pDebug));

        const UINT references = m_pDevice->Release();
        if (pDebug && references > 1)
            pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

        SAFE_RELEASE(pDebug);
        m_pDevice = nullptr;
    }
#else
    SAFE_RELEASE(m_pDevice);
#endif
}

bool DeviceResources::IsReady() const
{
    return m_pDevice != nullptr &&
        m_pDeviceContext != nullptr &&
        m_pSwapChain != nullptr &&
        m_pBackBufferRTV != nullptr &&
        m_pDepthStencilView != nullptr &&
        m_pColorBufferRTV != nullptr &&
        m_pColorBufferSRV != nullptr;
}

ID3D11Device* DeviceResources::GetDevice() const
{
    return m_pDevice;
}

ID3D11DeviceContext* DeviceResources::GetContext() const
{
    return m_pDeviceContext;
}

ID3D11RenderTargetView* DeviceResources::GetBackBufferRTV() const
{
    return m_pBackBufferRTV;
}

ID3D11ShaderResourceView* DeviceResources::GetColorBufferSRV() const
{
    return m_pColorBufferSRV;
}

UINT DeviceResources::GetWidth() const
{
    return m_Width;
}

UINT DeviceResources::GetHeight() const
{
    return m_Height;
}