#include "framework.h"
#include "SkyboxObject.h"

#include "DdsLoader.h"
#include "MeshUtils.h"
#include "ShaderUtils.h"

#include <array>
#include <string>

bool SkyboxObject::Init(ID3D11Device* pDevice)
{
    return CreateGeometry(pDevice) &&
        CreateShaders(pDevice) &&
        CreateCubemap(pDevice) &&
        CreateSampler(pDevice) &&
        CreateState(pDevice);
}

bool SkyboxObject::CreateGeometry(ID3D11Device* pDevice)
{
    std::vector<SkyVertex> vertices;
    std::vector<UINT16> indices;
    BuildSphereMesh(vertices, indices);

    m_Geometry.indexCount = static_cast<UINT>(indices.size());

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(SkyVertex));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = vertices.data();
        data.SysMemPitch = desc.ByteWidth;

        const HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_Geometry.pVertexBuffer);
        if (FAILED(hr) || !m_Geometry.pVertexBuffer)
            return false;

        SetResourceName(m_Geometry.pVertexBuffer, "SkyVertexBuffer");
    }

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = static_cast<UINT>(indices.size() * sizeof(UINT16));
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA data{};
        data.pSysMem = indices.data();
        data.SysMemPitch = desc.ByteWidth;

        const HRESULT hr = pDevice->CreateBuffer(&desc, &data, &m_Geometry.pIndexBuffer);
        if (FAILED(hr) || !m_Geometry.pIndexBuffer)
            return false;

        SetResourceName(m_Geometry.pIndexBuffer, "SkyIndexBuffer");
    }

    return true;
}

bool SkyboxObject::CreateShaders(ID3D11Device* pDevice)
{
    {
        ID3DBlob* pVSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"Skybox.vs", "vs", "vs_5_0", &pVSCode);
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

        SetResourceName(m_Material.pVertexShader, "Skybox.vs");

        static const D3D11_INPUT_ELEMENT_DESC InputDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = pDevice->CreateInputLayout(
            InputDesc,
            1,
            pVSCode->GetBufferPointer(),
            pVSCode->GetBufferSize(),
            &m_Material.pInputLayout);

        SAFE_RELEASE(pVSCode);

        if (FAILED(hr) || !m_Material.pInputLayout)
            return false;

        SetResourceName(m_Material.pInputLayout, "SkyInputLayout");
    }

    {
        ID3DBlob* pPSCode = nullptr;
        const HRESULT hrCompile = CompileShaderFromFileMemory(L"Skybox.ps", "ps", "ps_5_0", &pPSCode);
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

        SetResourceName(m_Material.pPixelShader, "Skybox.ps");
    }

    return true;
}

bool SkyboxObject::CreateCubemap(ID3D11Device* pDevice)
{
    static const std::wstring TextureNames[6] =
    {
        L"posx.dds", L"negx.dds",
        L"posy.dds", L"negy.dds",
        L"posz.dds", L"negz.dds"
    };

    std::array<TextureDesc, 6> texDescs;
    for (int i = 0; i < 6; ++i)
    {
        if (!LoadDDS(TextureNames[i].c_str(), texDescs[i]))
            return false;
    }

    // Все 6 граней должны совпадать по формату, размеру и числу mip-уровней
    for (int i = 1; i < 6; ++i)
    {
        if (texDescs[i].fmt != texDescs[0].fmt ||
            texDescs[i].width != texDescs[0].width ||
            texDescs[i].height != texDescs[0].height ||
            texDescs[i].mipmapsCount != texDescs[0].mipmapsCount)
        {
            return false;
        }
    }

    const UINT mipLevels = texDescs[0].mipmapsCount;

    D3D11_TEXTURE2D_DESC desc{};
    desc.Format = texDescs[0].fmt;
    desc.ArraySize = 6;
    desc.MipLevels = mipLevels;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width = texDescs[0].width;
    desc.Height = texDescs[0].height;

    std::vector<D3D11_SUBRESOURCE_DATA> subresources(6 * mipLevels);

    for (UINT face = 0; face < 6; ++face)
    {
        std::vector<D3D11_SUBRESOURCE_DATA> faceMipData;
        FillTextureSubresourceData(
            texDescs[face].fmt,
            texDescs[face].width,
            texDescs[face].height,
            texDescs[face].mipmapsCount,
            texDescs[face].data.data(),
            faceMipData);

        for (UINT mip = 0; mip < mipLevels; ++mip)
        {
            const UINT subresourceIndex = D3D11CalcSubresource(mip, face, mipLevels);
            subresources[subresourceIndex] = faceMipData[mip];
        }
    }

    HRESULT hr = pDevice->CreateTexture2D(&desc, subresources.data(), &m_Material.pCubemapTexture);
    if (FAILED(hr) || !m_Material.pCubemapTexture)
        return false;

    SetResourceName(m_Material.pCubemapTexture, "CubemapTexture");

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = mipLevels;

    hr = pDevice->CreateShaderResourceView(
        m_Material.pCubemapTexture,
        &srvDesc,
        &m_Material.pCubemapView);

    if (FAILED(hr) || !m_Material.pCubemapView)
        return false;

    SetResourceName(m_Material.pCubemapView, "CubemapSRV");
    return true;
}

bool SkyboxObject::CreateSampler(ID3D11Device* pDevice)
{
    D3D11_SAMPLER_DESC desc{};
    desc.Filter = D3D11_FILTER_ANISOTROPIC;
    desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    desc.MinLOD = -FLT_MAX;
    desc.MaxLOD = FLT_MAX;
    desc.MipLODBias = 0.0f;
    desc.MaxAnisotropy = 16;
    desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    desc.BorderColor[0] = 1.0f;
    desc.BorderColor[1] = 1.0f;
    desc.BorderColor[2] = 1.0f;
    desc.BorderColor[3] = 1.0f;

    const HRESULT hr = pDevice->CreateSamplerState(&desc, &m_Material.pSampler);
    if (FAILED(hr) || !m_Material.pSampler)
        return false;

    SetResourceName(m_Material.pSampler, "AnisotropicWrapSampler");
    return true;
}

bool SkyboxObject::CreateState(ID3D11Device* pDevice)
{
    {
        D3D11_RASTERIZER_DESC desc{};
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = FALSE;
        desc.DepthClipEnable = TRUE;

        const HRESULT hr = pDevice->CreateRasterizerState(&desc, &m_State.pRasterizerState);
        if (FAILED(hr) || !m_State.pRasterizerState)
            return false;

        SetResourceName(m_State.pRasterizerState, "SkyRasterizerState");
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        desc.StencilEnable = FALSE;

        const HRESULT hr = pDevice->CreateDepthStencilState(&desc, &m_State.pDepthState);
        if (FAILED(hr) || !m_State.pDepthState)
            return false;

        SetResourceName(m_State.pDepthState, "SkyDepthState");
    }

    return true;
}

bool SkyboxObject::IsReady() const
{
    return m_Geometry.pVertexBuffer != nullptr &&
        m_Geometry.pIndexBuffer != nullptr &&
        m_Material.pVertexShader != nullptr &&
        m_Material.pPixelShader != nullptr &&
        m_Material.pInputLayout != nullptr &&
        m_Material.pCubemapView != nullptr &&
        m_Material.pSampler != nullptr &&
        m_State.pRasterizerState != nullptr &&
        m_State.pDepthState != nullptr;
}

void SkyboxObject::Render(
    ID3D11DeviceContext* pDeviceContext,
    ID3D11Buffer* pGeomBuffer,
    ID3D11Buffer* pSceneBuffer)
{
    if (!IsReady())
        return;

    pDeviceContext->IASetIndexBuffer(m_Geometry.pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vertexBuffers[] = { m_Geometry.pVertexBuffer };
    UINT strides[] = { sizeof(SkyVertex) };
    UINT offsets[] = { 0 };
    pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

    pDeviceContext->IASetInputLayout(m_Material.pInputLayout);
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDeviceContext->VSSetShader(m_Material.pVertexShader, nullptr, 0);
    pDeviceContext->PSSetShader(m_Material.pPixelShader, nullptr, 0);

    ID3D11Buffer* vsConstantBuffers[] = { pSceneBuffer, pGeomBuffer };
    pDeviceContext->VSSetConstantBuffers(0, 2, vsConstantBuffers);

    ID3D11SamplerState* samplers[] = { m_Material.pSampler };
    pDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_Material.pCubemapView };
    pDeviceContext->PSSetShaderResources(0, 1, resources);

    pDeviceContext->RSSetState(m_State.pRasterizerState);
    pDeviceContext->OMSetDepthStencilState(m_State.pDepthState, 0);

    pDeviceContext->DrawIndexed(m_Geometry.indexCount, 0, 0);
}

void SkyboxObject::Shutdown()
{
    SAFE_RELEASE(m_State.pDepthState);
    SAFE_RELEASE(m_State.pRasterizerState);

    SAFE_RELEASE(m_Material.pSampler);
    SAFE_RELEASE(m_Material.pCubemapView);
    SAFE_RELEASE(m_Material.pCubemapTexture);
    SAFE_RELEASE(m_Material.pInputLayout);
    SAFE_RELEASE(m_Material.pPixelShader);
    SAFE_RELEASE(m_Material.pVertexShader);

    SAFE_RELEASE(m_Geometry.pIndexBuffer);
    SAFE_RELEASE(m_Geometry.pVertexBuffer);

    m_Geometry.indexCount = 0;
}