#include "framework.h"
#include "CubeObject.h"

#include "DdsLoader.h"
#include "ShaderUtils.h"

#include <array>
#include <vector>

namespace
{
    bool CreateTextureFromDDS(
        ID3D11Device* pDevice,
        const wchar_t* path,
        const char* textureDebugName,
        const char* srvDebugName,
        ID3D11Texture2D** ppTexture,
        ID3D11ShaderResourceView** ppTextureView)
    {
        if (!pDevice || !ppTexture || !ppTextureView)
            return false;

        *ppTexture = nullptr;
        *ppTextureView = nullptr;

        TextureDesc textureDesc;
        if (!LoadDDS(path, textureDesc))
            return false;

        D3D11_TEXTURE2D_DESC desc{};
        desc.Format = textureDesc.fmt;
        desc.ArraySize = 1;
        desc.MipLevels = textureDesc.mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Width = textureDesc.width;
        desc.Height = textureDesc.height;

        std::vector<D3D11_SUBRESOURCE_DATA> subresources;
        FillTextureSubresourceData(
            textureDesc.fmt,
            textureDesc.width,
            textureDesc.height,
            textureDesc.mipmapsCount,
            textureDesc.data.data(),
            subresources);

        HRESULT hr = pDevice->CreateTexture2D(&desc, subresources.data(), ppTexture);
        if (FAILED(hr) || !*ppTexture)
            return false;

        SetResourceName(*ppTexture, textureDebugName);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.fmt;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = textureDesc.mipmapsCount;
        srvDesc.Texture2D.MostDetailedMip = 0;

        hr = pDevice->CreateShaderResourceView(*ppTexture, &srvDesc, ppTextureView);
        if (FAILED(hr) || !*ppTextureView)
        {
            SAFE_RELEASE(*ppTexture);
            return false;
        }

        SetResourceName(*ppTextureView, srvDebugName);
        return true;
    }

    bool CreateTextureArrayFromDDS(
        ID3D11Device* pDevice,
        const std::array<std::wstring, 2>& textureNames,
        const char* textureDebugName,
        const char* srvDebugName,
        ID3D11Texture2D** ppTexture,
        ID3D11ShaderResourceView** ppTextureView)
    {
        if (!pDevice || !ppTexture || !ppTextureView)
            return false;

        *ppTexture = nullptr;
        *ppTextureView = nullptr;

        std::array<TextureDesc, 2> textureDesc;
        for (size_t i = 0; i < textureNames.size(); ++i)
        {
            if (!LoadDDS(textureNames[i].c_str(), textureDesc[i]))
                return false;
        }

        for (size_t i = 1; i < textureDesc.size(); ++i)
        {
            if (textureDesc[i].fmt != textureDesc[0].fmt ||
                textureDesc[i].width != textureDesc[0].width ||
                textureDesc[i].height != textureDesc[0].height ||
                textureDesc[i].mipmapsCount != textureDesc[0].mipmapsCount)
            {
                return false;
            }
        }

        D3D11_TEXTURE2D_DESC desc{};
        desc.Format = textureDesc[0].fmt;
        desc.ArraySize = 2;
        desc.MipLevels = textureDesc[0].mipmapsCount;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Width = textureDesc[0].width;
        desc.Height = textureDesc[0].height;

        std::vector<D3D11_SUBRESOURCE_DATA> subresources(desc.ArraySize * desc.MipLevels);

        for (UINT slice = 0; slice < desc.ArraySize; ++slice)
        {
            std::vector<D3D11_SUBRESOURCE_DATA> mipData;
            FillTextureSubresourceData(
                textureDesc[slice].fmt,
                textureDesc[slice].width,
                textureDesc[slice].height,
                textureDesc[slice].mipmapsCount,
                textureDesc[slice].data.data(),
                mipData);

            for (UINT mip = 0; mip < desc.MipLevels; ++mip)
            {
                const UINT subresourceIndex = D3D11CalcSubresource(mip, slice, desc.MipLevels);
                subresources[subresourceIndex] = mipData[mip];
            }
        }

        HRESULT hr = pDevice->CreateTexture2D(&desc, subresources.data(), ppTexture);
        if (FAILED(hr) || !*ppTexture)
            return false;

        SetResourceName(*ppTexture, textureDebugName);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.ArraySize = 2;
        srvDesc.Texture2DArray.FirstArraySlice = 0;
        srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
        srvDesc.Texture2DArray.MostDetailedMip = 0;

        hr = pDevice->CreateShaderResourceView(*ppTexture, &srvDesc, ppTextureView);
        if (FAILED(hr) || !*ppTextureView)
        {
            SAFE_RELEASE(*ppTexture);
            return false;
        }

        SetResourceName(*ppTextureView, srvDebugName);
        return true;
    }
}

bool CubeObject::Init(ID3D11Device* pDevice)
{
    return CreateGeometry(pDevice) &&
        CreateShaders(pDevice) &&
        CreateTextures(pDevice) &&
        CreateSampler(pDevice);
}

bool CubeObject::CreateGeometry(ID3D11Device* pDevice)
{
    static const TextureTangentVertex Vertices[24] =
    {
        { -0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f },
        {  0.5f, -0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f },
        {  0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f },
        { -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f },

        {  0.5f, -0.5f, -0.5f,  0.0f, 0.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f },
        {  0.5f, -0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f,  0.0f, 0.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f, -0.5f,  0.0f, 0.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f },

        {  0.5f, -0.5f,  0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f,  0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f },
        {  0.5f,  0.5f,  0.5f, -1.0f, 0.0f,  0.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f },

        { -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f },
        { -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f },

        { -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f },

        { -0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f },
        {  0.5f, -0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f },
        { -0.5f,  0.5f, -0.5f,  1.0f, 0.0f,  0.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f }
    };

    static const UINT16 Indices[36] =
    {
         0,  2,  1,   0,  3,  2,
         4,  6,  5,   4,  7,  6,
         8, 10,  9,   8, 11, 10,
        12, 14, 13,  12, 15, 14,
        16, 17, 18,  16, 18, 19,
        20, 22, 21,  20, 23, 22
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

        SetResourceName(m_Geometry.pVertexBuffer, "CubeVertexBuffer");
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

        SetResourceName(m_Geometry.pIndexBuffer, "CubeIndexBuffer");
    }

    return true;
}

bool CubeObject::CreateShaders(ID3D11Device* pDevice)
{
    {
        ID3DBlob* pVSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"LitCube.vs", "vs", "vs_5_0", &pVSCode);
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

        SetResourceName(m_Material.pVertexShader, "LitCube.vs");

        static const D3D11_INPUT_ELEMENT_DESC InputDesc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        hr = pDevice->CreateInputLayout(
            InputDesc,
            4,
            pVSCode->GetBufferPointer(),
            pVSCode->GetBufferSize(),
            &m_Material.pInputLayout);

        SAFE_RELEASE(pVSCode);

        if (FAILED(hr) || !m_Material.pInputLayout)
            return false;

        SetResourceName(m_Material.pInputLayout, "CubeInputLayout");
    }

    {
        ID3DBlob* pPSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"LitCube.ps", "ps", "ps_5_0", &pPSCode);
        if (FAILED(hr) || !pPSCode)
            return false;

        hr = pDevice->CreatePixelShader(
            pPSCode->GetBufferPointer(),
            pPSCode->GetBufferSize(),
            nullptr,
            &m_Material.pPixelShader);

        SAFE_RELEASE(pPSCode);

        if (FAILED(hr) || !m_Material.pPixelShader)
            return false;

        SetResourceName(m_Material.pPixelShader, "LitCube.ps");
    }

    return true;
}

bool CubeObject::CreateTextures(ID3D11Device* pDevice)
{
    const std::array<std::wstring, 2> colorTextures =
    {
        L"Brick.dds",
        L"Kitty.dds"
    };

    return CreateTextureArrayFromDDS(
        pDevice,
        colorTextures,
        "CubeColorTextureArray",
        "CubeColorTextureArraySRV",
        &m_Material.pColorTextureArray,
        &m_Material.pColorTextureArrayView) &&
        CreateTextureFromDDS(
            pDevice,
            L"BrickNM.dds",
            "CubeNormalTexture",
            "CubeNormalTextureSRV",
            &m_Material.pNormalTexture,
            &m_Material.pNormalTextureView);
}

bool CubeObject::CreateSampler(ID3D11Device* pDevice)
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

bool CubeObject::IsReady() const
{
    return m_Geometry.pVertexBuffer != nullptr &&
        m_Geometry.pIndexBuffer != nullptr &&
        m_Material.pVertexShader != nullptr &&
        m_Material.pPixelShader != nullptr &&
        m_Material.pInputLayout != nullptr &&
        m_Material.pColorTextureArrayView != nullptr &&
        m_Material.pNormalTextureView != nullptr &&
        m_Material.pSampler != nullptr;
}

void CubeObject::Render(
    ID3D11DeviceContext* pDeviceContext,
    ID3D11Buffer* pSceneBuffer,
    ID3D11Buffer* pGeomBufferInst,
    ID3D11Buffer* pGeomBufferInstVis,
    UINT instanceCount,
    ID3D11Buffer* pIndirectArgs)
{
    if (!IsReady())
        return;

    if (instanceCount == 0 && pIndirectArgs == nullptr)
        return;

    pDeviceContext->IASetIndexBuffer(m_Geometry.pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    ID3D11Buffer* vertexBuffers[] = { m_Geometry.pVertexBuffer };
    UINT strides[] = { sizeof(TextureTangentVertex) };
    UINT offsets[] = { 0 };
    pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

    pDeviceContext->IASetInputLayout(m_Material.pInputLayout);
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDeviceContext->VSSetShader(m_Material.pVertexShader, nullptr, 0);
    pDeviceContext->PSSetShader(m_Material.pPixelShader, nullptr, 0);

    ID3D11Buffer* vsConstantBuffers[] =
    {
        pSceneBuffer,
        pGeomBufferInst,
        pGeomBufferInstVis
    };
    pDeviceContext->VSSetConstantBuffers(0, 3, vsConstantBuffers);

    ID3D11Buffer* psConstantBuffers[] =
    {
        pSceneBuffer,
        pGeomBufferInst,
        pGeomBufferInstVis
    };
    pDeviceContext->PSSetConstantBuffers(0, 3, psConstantBuffers);

    ID3D11SamplerState* samplers[] = { m_Material.pSampler };
    pDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] =
    {
        m_Material.pColorTextureArrayView,
        m_Material.pNormalTextureView
    };
    pDeviceContext->PSSetShaderResources(0, 2, resources);

    pDeviceContext->RSSetState(nullptr);

    if (pIndirectArgs != nullptr)
        pDeviceContext->DrawIndexedInstancedIndirect(pIndirectArgs, 0);
    else
        pDeviceContext->DrawIndexedInstanced(m_Geometry.indexCount, instanceCount, 0, 0, 0);
}

void CubeObject::Shutdown()
{
    SAFE_RELEASE(m_Material.pSampler);

    SAFE_RELEASE(m_Material.pNormalTextureView);
    SAFE_RELEASE(m_Material.pNormalTexture);

    SAFE_RELEASE(m_Material.pColorTextureArrayView);
    SAFE_RELEASE(m_Material.pColorTextureArray);

    SAFE_RELEASE(m_Material.pInputLayout);
    SAFE_RELEASE(m_Material.pPixelShader);
    SAFE_RELEASE(m_Material.pVertexShader);

    SAFE_RELEASE(m_Geometry.pIndexBuffer);
    SAFE_RELEASE(m_Geometry.pVertexBuffer);

    m_Geometry.indexCount = 0;
}