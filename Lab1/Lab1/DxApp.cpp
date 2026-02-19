#include "framework.h"
#include "DxApp.h"
#include <assert.h>

#include <d3dcompiler.h>
#include <d3dcommon.h>

#include <string>
#include <vector>
#include <fstream>

#pragma comment(lib, "d3dcompiler.lib")

static inline HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name)
{
    if (!pResource) return E_INVALIDARG;
    return pResource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.length(), name.c_str());
}

static std::string WCSToMBS(const std::wstring& w)
{
    if (w.empty()) return {};

    int sizeNeeded = WideCharToMultiByte(
        CP_UTF8, 0,
        w.c_str(), (int)w.size(),
        nullptr, 0,
        nullptr, nullptr
    );

    if (sizeNeeded <= 0) return {};

    std::string result;
    result.resize(sizeNeeded);

    WideCharToMultiByte(
        CP_UTF8, 0,
        w.c_str(), (int)w.size(),
        &result[0], sizeNeeded,   
        nullptr, nullptr
    );

    return result;
}


static bool ReadAllBytes(const std::wstring& path, std::vector<char>& out)
{
    out.clear();

    std::ifstream file(WCSToMBS(path), std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    std::streamoff size = file.tellg();
    if (size <= 0) return false;
    file.seekg(0, std::ios::beg);

    out.resize((size_t)size);
    file.read(out.data(), size);
    file.close();


    out.push_back('\0');
    return true;
}

static HRESULT CompileShaderFromFileMemory(
    const std::wstring& path,
    const std::string& entryPoint,
    const std::string& platform,
    ID3DBlob** ppCode)
{
    if (!ppCode) return E_INVALIDARG;
    *ppCode = nullptr;

    std::vector<char> data;
    if (!ReadAllBytes(path, data))
        return E_FAIL;

    UINT flags1 = 0;
#if defined(_DEBUG)
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrMsg = nullptr;

    HRESULT result = D3DCompile(
        data.data(),
        data.size(),
        WCSToMBS(path).c_str(),
        nullptr,
        nullptr,
        entryPoint.c_str(),
        platform.c_str(),
        flags1,
        0,
        ppCode,
        &pErrMsg
    );

    if (!SUCCEEDED(result) && pErrMsg != nullptr)
        OutputDebugStringA((const char*)pErrMsg->GetBufferPointer());

    assert(SUCCEEDED(result));
    SAFE_RELEASE(pErrMsg);

    return result;
}

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

    
    if (!CreateTriangleResources())
        return false;

    return true;
}

void DxApp::Shutdown()
{
    ReleaseTriangleResources();
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

bool DxApp::CreateTriangleResources()
{
    if (!m_pDevice) return false;

    struct Vertex
    {
        float x, y, z;
        COLORREF color;
    };

    static const Vertex Vertices[] = {
        {-0.5f, -0.5f, 0.0f, RGB(255, 0, 0)},
        { 0.5f, -0.5f, 0.0f, RGB(0, 255, 0)},
        { 0.0f,  0.5f, 0.0f, RGB(0, 0, 255)}
    };

    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Vertices;
        data.SysMemPitch = sizeof(Vertices);
        data.SysMemSlicePitch = 0;

        HRESULT result = m_pDevice->CreateBuffer(&desc, &data, &m_pVertexBuffer);
        if (FAILED(result) || !m_pVertexBuffer) return false;
        SetResourceName(m_pVertexBuffer, "VertexBuffer");
    }

    static const USHORT Indices[] = { 0, 2, 1 };

    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        HRESULT result = m_pDevice->CreateBuffer(&desc, &data, &m_pIndexBuffer);
        if (FAILED(result) || !m_pIndexBuffer) return false;
        SetResourceName(m_pIndexBuffer, "IndexBuffer");
    }


    ID3DBlob* pVSCode = nullptr;
    HRESULT result = CompileShaderFromFileMemory(L"Triangle.vs", "vs", "vs_5_0", &pVSCode);
    if (FAILED(result) || !pVSCode) return false;

    result = m_pDevice->CreateVertexShader(
        pVSCode->GetBufferPointer(),
        pVSCode->GetBufferSize(),
        nullptr,
        &m_pVertexShader
    );
    if (FAILED(result) || !m_pVertexShader)
    {
        SAFE_RELEASE(pVSCode);
        return false;
    }
    SetResourceName(m_pVertexShader, "Triangle.vs");

    ID3DBlob* pPSCode = nullptr;
    result = CompileShaderFromFileMemory(L"Triangle.ps", "ps", "ps_5_0", &pPSCode);
    if (FAILED(result) || !pPSCode)
    {
        SAFE_RELEASE(pVSCode);
        return false;
    }

    result = m_pDevice->CreatePixelShader(
        pPSCode->GetBufferPointer(),
        pPSCode->GetBufferSize(),
        nullptr,
        &m_pPixelShader
    );
    SAFE_RELEASE(pPSCode);

    if (FAILED(result) || !m_pPixelShader)
    {
        SAFE_RELEASE(pVSCode);
        return false;
    }
    SetResourceName(m_pPixelShader, "Triangle.ps");


    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    result = m_pDevice->CreateInputLayout(
        InputDesc,
        2,
        pVSCode->GetBufferPointer(),
        pVSCode->GetBufferSize(),
        &m_pInputLayout
    );

    SAFE_RELEASE(pVSCode);

    if (FAILED(result) || !m_pInputLayout)
        return false;

    SetResourceName(m_pInputLayout, "InputLayout");

    return true;
}

void DxApp::ReleaseTriangleResources()
{
    SAFE_RELEASE(m_pInputLayout);
    SAFE_RELEASE(m_pPixelShader);
    SAFE_RELEASE(m_pVertexShader);
    SAFE_RELEASE(m_pIndexBuffer);
    SAFE_RELEASE(m_pVertexBuffer);
}

void DxApp::Render()
{
    if (!m_pDeviceContext || !m_pBackBufferRTV || !m_pSwapChain) return;

   
    if (!m_pVertexBuffer || !m_pIndexBuffer || !m_pVertexShader || !m_pPixelShader || !m_pInputLayout)
    {
        ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
        m_pDeviceContext->ClearState();
        m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);
        m_pDeviceContext->ClearRenderTargetView(views[0], m_ClearColor);
        HRESULT hr = m_pSwapChain->Present(0, 0);
        assert(SUCCEEDED(hr));
        return;
    }

  
    m_pDeviceContext->ClearState();

    ID3D11RenderTargetView* views[] = { m_pBackBufferRTV };
    m_pDeviceContext->OMSetRenderTargets(1, views, nullptr);
    m_pDeviceContext->ClearRenderTargetView(m_pBackBufferRTV, m_ClearColor);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)m_Width;
    viewport.Height = (FLOAT)m_Height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_pDeviceContext->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = (LONG)m_Width;
    rect.bottom = (LONG)m_Height;
    m_pDeviceContext->RSSetScissorRects(1, &rect);

    m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vertexBuffers[] = { m_pVertexBuffer };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

    m_pDeviceContext->IASetInputLayout(m_pInputLayout);
    m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    m_pDeviceContext->DrawIndexed(3, 0, 0);

    HRESULT hr = m_pSwapChain->Present(0, 0);
    assert(SUCCEEDED(hr));
}
