#include "framework.h"
#include "TransparentQuadObject.h"

#include "ShaderUtils.h"

bool TransparentQuadObject::Init(ID3D11Device* pDevice)
{
    return CreateGeometry(pDevice) &&
        CreateShaders(pDevice) &&
        CreateState(pDevice);
}

bool TransparentQuadObject::CreateGeometry(ID3D11Device* pDevice)
{
    static const TextureVertex Vertices[4] =
    {
        { -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
        { -0.5f,  0.5f, 0.0f, 0.0f, 0.0f },
        {  0.5f,  0.5f, 0.0f, 0.0f, 0.0f },
        {  0.5f, -0.5f, 0.0f, 0.0f, 1.0f }
    };

    static const UINT16 Indices[6] =
    {
        0, 1, 2,
        0, 2, 3
    };

    m_Geometry.indexCount = static_cast<UINT>(sizeof(Indices) / sizeof(Indices[0]));

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(Vertices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = Vertices;
        data.SysMemPitch = sizeof(Vertices);

        const HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_Geometry.pVertexBuffer);
        if (FAILED(hr) || !m_Geometry.pVertexBuffer)
            return false;

        SetResourceName(m_Geometry.pVertexBuffer, "TransparentQuadVertexBuffer");
    }

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = Indices;
        data.SysMemPitch = sizeof(Indices);

        const HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_Geometry.pIndexBuffer);
        if (FAILED(hr) || !m_Geometry.pIndexBuffer)
            return false;

        SetResourceName(m_Geometry.pIndexBuffer, "TransparentQuadIndexBuffer");
    }

    return true;
}

bool TransparentQuadObject::CreateShaders(ID3D11Device* pDevice)
{
    {
        ID3DBlob* pVSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"Triangle.vs", "vs", "vs_5_0", &pVSCode);
        if (FAILED(hr) || !pVSCode)
            return false;

        hr = pDevice->CreateVertexShader(
            pVSCode->GetBufferPointer(),
            pVSCode->GetBufferSize(),
            nullptr,
            &m_Material.pVertexShader);
        if (FAILED(hr) || !m_Material.pVertexShader)
        {
            SAFE_RELEASE(pVSCode);
            return false;
        }

        SetResourceName(m_Material.pVertexShader, "Triangle.vs for transparent quad");

        static const D3D11_INPUT_ELEMENT_DESC InputDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = pDevice->CreateInputLayout(
            InputDesc,
            2,
            pVSCode->GetBufferPointer(),
            pVSCode->GetBufferSize(),
            &m_Material.pInputLayout);

        SAFE_RELEASE(pVSCode);

        if (FAILED(hr) || !m_Material.pInputLayout)
            return false;

        SetResourceName(m_Material.pInputLayout, "TransparentQuadInputLayout");
    }

    {
        ID3DBlob* pPSCode = nullptr;
        const HRESULT hrCompile = CompileShaderFromFileMemory(L"Transparent.ps", "ps", "ps_5_0", &pPSCode);
        if (FAILED(hrCompile) || !pPSCode)
            return false;

        const HRESULT hr = pDevice->CreatePixelShader(
            pPSCode->GetBufferPointer(),
            pPSCode->GetBufferSize(),
            nullptr,
            &m_Material.pPixelShader);

        SAFE_RELEASE(pPSCode);

        if (FAILED(hr) || !m_Material.pPixelShader)
            return false;

        SetResourceName(m_Material.pPixelShader, "Transparent.ps");
    }

    return true;
}

bool TransparentQuadObject::CreateState(ID3D11Device* pDevice)
{
    D3D11_RASTERIZER_DESC desc{};
    desc.FillMode = D3D11_FILL_SOLID;
    desc.CullMode = D3D11_CULL_NONE;
    desc.FrontCounterClockwise = FALSE;
    desc.DepthClipEnable = TRUE;

    const HRESULT hr = pDevice->CreateRasterizerState(&desc, &m_State.pRasterizerState);
    if (FAILED(hr) || !m_State.pRasterizerState)
        return false;

    return true;
}

bool TransparentQuadObject::IsReady() const
{
    return m_Geometry.pVertexBuffer != nullptr &&
        m_Geometry.pIndexBuffer != nullptr &&
        m_Material.pVertexShader != nullptr &&
        m_Material.pPixelShader != nullptr &&
        m_Material.pInputLayout != nullptr &&
        m_State.pRasterizerState != nullptr;
}

void TransparentQuadObject::Render(
    ID3D11DeviceContext* pDeviceContext,
    ID3D11Buffer* pGeomBuffer,
    ID3D11Buffer* pSceneBuffer)
{
    if (!IsReady())
        return;

    pDeviceContext->IASetIndexBuffer(m_Geometry.pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vertexBuffers[] = { m_Geometry.pVertexBuffer };
    UINT strides[] = { sizeof(TextureVertex) };
    UINT offsets[] = { 0 };
    pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

    pDeviceContext->IASetInputLayout(m_Material.pInputLayout);
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDeviceContext->VSSetShader(m_Material.pVertexShader, nullptr, 0);
    pDeviceContext->PSSetShader(m_Material.pPixelShader, nullptr, 0);

    ID3D11Buffer* vsConstantBuffers[] = { pGeomBuffer, pSceneBuffer };
    pDeviceContext->VSSetConstantBuffers(0, 2, vsConstantBuffers);

    ID3D11Buffer* psConstantBuffers[] = { pGeomBuffer };
    pDeviceContext->PSSetConstantBuffers(0, 1, psConstantBuffers);

    pDeviceContext->RSSetState(m_State.pRasterizerState);
    pDeviceContext->DrawIndexed(m_Geometry.indexCount, 0, 0);
}

void TransparentQuadObject::Shutdown()
{
    SAFE_RELEASE(m_State.pRasterizerState);

    SAFE_RELEASE(m_Material.pInputLayout);
    SAFE_RELEASE(m_Material.pPixelShader);
    SAFE_RELEASE(m_Material.pVertexShader);

    SAFE_RELEASE(m_Geometry.pIndexBuffer);
    SAFE_RELEASE(m_Geometry.pVertexBuffer);

    m_Geometry.indexCount = 0;
}