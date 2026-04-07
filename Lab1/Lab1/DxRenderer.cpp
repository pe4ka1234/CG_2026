#include "framework.h"
#include "DxRenderer.h"

#include "ShaderUtils.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cfloat>
#include <vector>

namespace
{
    struct TransparentInstance
    {
        DirectX::XMMATRIX model;
        DirectX::XMFLOAT4 color;
        float sortKey = 0.0f;
    };

    struct OpaqueInstanceDesc
    {
        DirectX::XMFLOAT3 position;
        float scale;
        float angle;
        float shininess;
        float textureId;
        float hasNormalMap;
    };

    float ComputeTransparentSortKey(
        const DirectX::XMMATRIX& model,
        const DirectX::XMFLOAT3& cameraPos)
    {
        static const DirectX::XMVECTOR corners[4] =
        {
            DirectX::XMVectorSet(-0.5f, -0.5f, 0.0f, 1.0f),
            DirectX::XMVectorSet(-0.5f,  0.5f, 0.0f, 1.0f),
            DirectX::XMVectorSet(0.5f,  0.5f, 0.0f, 1.0f),
            DirectX::XMVectorSet(0.5f, -0.5f, 0.0f, 1.0f)
        };

        const DirectX::XMVECTOR camera = DirectX::XMVectorSet(
            cameraPos.x,
            cameraPos.y,
            cameraPos.z,
            1.0f);

        float maxDistSq = 0.0f;
        for (const DirectX::XMVECTOR& corner : corners)
        {
            const DirectX::XMVECTOR worldCorner = DirectX::XMVector3TransformCoord(corner, model);
            const DirectX::XMVECTOR diff = DirectX::XMVectorSubtract(worldCorner, camera);
            const float distSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(diff));
            if (distSq > maxDistSq)
                maxDistSq = distSq;
        }

        return maxDistSq;
    }

    void BuildWorldAABB(
        const DirectX::XMMATRIX& model,
        DirectX::XMFLOAT3& bbMin,
        DirectX::XMFLOAT3& bbMax)
    {
        static const DirectX::XMVECTOR corners[8] =
        {
            DirectX::XMVectorSet(-0.5f, -0.5f, -0.5f, 1.0f),
            DirectX::XMVectorSet(0.5f, -0.5f, -0.5f, 1.0f),
            DirectX::XMVectorSet(-0.5f,  0.5f, -0.5f, 1.0f),
            DirectX::XMVectorSet(0.5f,  0.5f, -0.5f, 1.0f),
            DirectX::XMVectorSet(-0.5f, -0.5f,  0.5f, 1.0f),
            DirectX::XMVectorSet(0.5f, -0.5f,  0.5f, 1.0f),
            DirectX::XMVectorSet(-0.5f,  0.5f,  0.5f, 1.0f),
            DirectX::XMVectorSet(0.5f,  0.5f,  0.5f, 1.0f)
        };

        DirectX::XMFLOAT3 minPt(FLT_MAX, FLT_MAX, FLT_MAX);
        DirectX::XMFLOAT3 maxPt(-FLT_MAX, -FLT_MAX, -FLT_MAX);

        for (const DirectX::XMVECTOR& corner : corners)
        {
            DirectX::XMFLOAT3 worldCorner;
            DirectX::XMStoreFloat3(
                &worldCorner,
                DirectX::XMVector3TransformCoord(corner, model));

            minPt.x = (std::min)(minPt.x, worldCorner.x);
            minPt.y = (std::min)(minPt.y, worldCorner.y);
            minPt.z = (std::min)(minPt.z, worldCorner.z);

            maxPt.x = (std::max)(maxPt.x, worldCorner.x);
            maxPt.y = (std::max)(maxPt.y, worldCorner.y);
            maxPt.z = (std::max)(maxPt.z, worldCorner.z);
        }

        bbMin = minPt;
        bbMax = maxPt;
    }

    bool IsBoxInside(
        const DirectX::XMFLOAT4 frustum[6],
        const DirectX::XMFLOAT3& bbMin,
        const DirectX::XMFLOAT3& bbMax)
    {
        for (int i = 0; i < 6; ++i)
        {
            const DirectX::XMFLOAT4& norm = frustum[i];
            const DirectX::XMFLOAT4 p(
                std::signbit(norm.x) ? bbMin.x : bbMax.x,
                std::signbit(norm.y) ? bbMin.y : bbMax.y,
                std::signbit(norm.z) ? bbMin.z : bbMax.z,
                1.0f);

            const float s = p.x * norm.x + p.y * norm.y + p.z * norm.z + p.w * norm.w;
            if (s < 0.0f)
                return false;
        }

        return true;
    }
}

bool DxRenderer::Init(HWND hWnd, UINT width, UINT height)
{
    if (!m_DeviceResources.Init(hWnd, width, height))
        return false;

    if (!m_SceneResources.Init(m_DeviceResources.GetDevice()))
    {
        Shutdown();
        return false;
    }

    if (!m_CubeObject.Init(m_DeviceResources.GetDevice()))
    {
        Shutdown();
        return false;
    }

    if (!m_SkyboxObject.Init(m_DeviceResources.GetDevice()))
    {
        Shutdown();
        return false;
    }

    if (!m_TransparentQuadObject.Init(m_DeviceResources.GetDevice()))
    {
        Shutdown();
        return false;
    }

    if (!CreateStates())
    {
        Shutdown();
        return false;
    }

    if (!CreatePostProcessResources())
    {
        Shutdown();
        return false;
    }

    return true;
}

bool DxRenderer::CreateStates()
{
    ID3D11Device* pDevice = m_DeviceResources.GetDevice();
    if (!pDevice)
        return false;

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_GREATER;
        desc.StencilEnable = FALSE;

        const HRESULT hr = pDevice->CreateDepthStencilState(&desc, &m_pOpaqueDepthState);
        if (FAILED(hr) || !m_pOpaqueDepthState)
            return false;
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc{};
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_GREATER;
        desc.StencilEnable = FALSE;

        const HRESULT hr = pDevice->CreateDepthStencilState(&desc, &m_pTransparentDepthState);
        if (FAILED(hr) || !m_pTransparentDepthState)
            return false;
    }

    {
        D3D11_BLEND_DESC desc{};
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = TRUE;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask =
            D3D11_COLOR_WRITE_ENABLE_RED |
            D3D11_COLOR_WRITE_ENABLE_GREEN |
            D3D11_COLOR_WRITE_ENABLE_BLUE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

        const HRESULT hr = pDevice->CreateBlendState(&desc, &m_pTransparentBlendState);
        if (FAILED(hr) || !m_pTransparentBlendState)
            return false;
    }

    return true;
}

void DxRenderer::ReleaseStates()
{
    SAFE_RELEASE(m_pTransparentBlendState);
    SAFE_RELEASE(m_pTransparentDepthState);
    SAFE_RELEASE(m_pOpaqueDepthState);
}

bool DxRenderer::CreatePostProcessResources()
{
    ID3D11Device* pDevice = m_DeviceResources.GetDevice();
    if (!pDevice)
        return false;

    {
        ID3DBlob* pVSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"Sepia.vs", "vs", "vs_5_0", &pVSCode);
        if (FAILED(hr) || !pVSCode)
            return false;

        hr = pDevice->CreateVertexShader(
            pVSCode->GetBufferPointer(),
            pVSCode->GetBufferSize(),
            nullptr,
            &m_pSepiaVertexShader);

        SAFE_RELEASE(pVSCode);

        if (FAILED(hr) || !m_pSepiaVertexShader)
            return false;

        SetResourceName(m_pSepiaVertexShader, "Sepia.vs");
    }

    {
        ID3DBlob* pPSCode = nullptr;
        HRESULT hr = CompileShaderFromFileMemory(L"Sepia.ps", "ps", "ps_5_0", &pPSCode);
        if (FAILED(hr) || !pPSCode)
            return false;

        hr = pDevice->CreatePixelShader(
            pPSCode->GetBufferPointer(),
            pPSCode->GetBufferSize(),
            nullptr,
            &m_pSepiaPixelShader);

        SAFE_RELEASE(pPSCode);

        if (FAILED(hr) || !m_pSepiaPixelShader)
            return false;

        SetResourceName(m_pSepiaPixelShader, "Sepia.ps");
    }

    {
        D3D11_SAMPLER_DESC desc{};
        desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD = 0.0f;
        desc.MaxLOD = FLT_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 1;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = 1.0f;
        desc.BorderColor[1] = 1.0f;
        desc.BorderColor[2] = 1.0f;
        desc.BorderColor[3] = 1.0f;

        const HRESULT hr = pDevice->CreateSamplerState(&desc, &m_pPostProcessSampler);
        if (FAILED(hr) || !m_pPostProcessSampler)
            return false;

        SetResourceName(m_pPostProcessSampler, "PostProcessSampler");
    }

    return true;
}

void DxRenderer::ReleasePostProcessResources()
{
    SAFE_RELEASE(m_pPostProcessSampler);
    SAFE_RELEASE(m_pSepiaPixelShader);
    SAFE_RELEASE(m_pSepiaVertexShader);
}

void DxRenderer::Shutdown()
{
    ReleasePostProcessResources();
    ReleaseStates();

    m_TransparentQuadObject.Shutdown();
    m_SkyboxObject.Shutdown();
    m_CubeObject.Shutdown();
    m_SceneResources.Shutdown();
    m_DeviceResources.Shutdown();
}

void DxRenderer::SetClearColor(float r, float g, float b, float a)
{
    m_ClearColor[0] = r;
    m_ClearColor[1] = g;
    m_ClearColor[2] = b;
    m_ClearColor[3] = a;
}

void DxRenderer::RotateCamera(float deltaYaw, float deltaPitch)
{
    m_SceneResources.RotateCamera(deltaYaw, deltaPitch);
}

void DxRenderer::RenderPostProcess()
{
    ID3D11DeviceContext* pDeviceContext = m_DeviceResources.GetContext();
    if (!pDeviceContext || !m_pSepiaVertexShader || !m_pSepiaPixelShader || !m_pPostProcessSampler)
        return;

    ID3D11RenderTargetView* views[] = { m_DeviceResources.GetBackBufferRTV() };
    pDeviceContext->OMSetRenderTargets(1, views, nullptr);

    ID3D11SamplerState* samplers[] = { m_pPostProcessSampler };
    pDeviceContext->PSSetSamplers(0, 1, samplers);

    ID3D11ShaderResourceView* resources[] = { m_DeviceResources.GetColorBufferSRV() };
    pDeviceContext->PSSetShaderResources(0, 1, resources);

    pDeviceContext->OMSetDepthStencilState(nullptr, 0);
    pDeviceContext->RSSetState(nullptr);
    pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    pDeviceContext->IASetInputLayout(nullptr);
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDeviceContext->VSSetShader(m_pSepiaVertexShader, nullptr, 0);
    pDeviceContext->PSSetShader(m_pSepiaPixelShader, nullptr, 0);

    pDeviceContext->Draw(6, 0);

    ID3D11ShaderResourceView* nullResources[] = { nullptr };
    pDeviceContext->PSSetShaderResources(0, 1, nullResources);
}

void DxRenderer::Render()
{
    if (!m_DeviceResources.IsReady())
        return;

    m_DeviceResources.BeginFrame(m_ClearColor);

    ID3D11DeviceContext* pDeviceContext = m_DeviceResources.GetContext();
    const UINT width = m_DeviceResources.GetWidth();
    const UINT height = m_DeviceResources.GetHeight();

    if (!m_SceneResources.UpdateSceneBuffer(pDeviceContext, width, height))
    {
        m_DeviceResources.Present();
        return;
    }

    static const DirectX::XMFLOAT4 zeroSize = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    static const DirectX::XMFLOAT4 whiteColor = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

    static const OpaqueInstanceDesc OpaqueInstances[] =
    {
        { DirectX::XMFLOAT3(-3.20f, 0.00f,  1.60f), 1.00f,  0.10f, 48.0f, 0.0f, 1.0f },
        { DirectX::XMFLOAT3(-1.20f, 0.00f,  0.20f), 0.85f, -0.25f, 40.0f, 1.0f, 0.0f },
        { DirectX::XMFLOAT3(1.10f, 0.05f, -0.10f), 0.95f,  0.35f, 56.0f, 0.0f, 1.0f },
        { DirectX::XMFLOAT3(3.00f, 0.00f,  1.20f), 1.10f, -0.15f, 36.0f, 1.0f, 0.0f },
        { DirectX::XMFLOAT3(0.00f, 0.00f,  3.00f), 0.75f,  0.55f, 52.0f, 0.0f, 1.0f },
        { DirectX::XMFLOAT3(6.50f, 0.00f,  0.00f), 1.00f,  0.00f, 48.0f, 1.0f, 0.0f },
        { DirectX::XMFLOAT3(-6.50f, 0.00f,  0.00f), 1.00f,  0.20f, 48.0f, 0.0f, 1.0f },
        { DirectX::XMFLOAT3(0.00f, 0.00f, -6.00f), 1.00f, -0.35f, 48.0f, 1.0f, 0.0f }
    };


    std::array<GeomBufferInstData, MaxInst> geomBufferInstData{};
    std::vector<UINT> visibleIds;
    visibleIds.reserve(MaxInst);

    const DirectX::XMFLOAT4* frustum = m_SceneResources.GetFrustum();

    const UINT opaqueCount = static_cast<UINT>(sizeof(OpaqueInstances) / sizeof(OpaqueInstances[0]));
    for (UINT i = 0; i < opaqueCount; ++i)
    {
        const OpaqueInstanceDesc& inst = OpaqueInstances[i];
        const float angle = inst.angle;

        const DirectX::XMMATRIX model =
            DirectX::XMMatrixScaling(inst.scale, inst.scale, inst.scale) *
            DirectX::XMMatrixRotationY(angle) *
            DirectX::XMMatrixTranslation(inst.position.x, inst.position.y, inst.position.z);

        const DirectX::XMMATRIX norm = DirectX::XMMatrixTranspose(
            DirectX::XMMatrixInverse(nullptr, model));

        DirectX::XMStoreFloat4x4(&geomBufferInstData[i].model, model);
        DirectX::XMStoreFloat4x4(&geomBufferInstData[i].norm, norm);

        geomBufferInstData[i].shineSpeedTexIdNM = DirectX::XMFLOAT4(
            inst.shininess,
            0.0f,
            inst.textureId,
            inst.hasNormalMap);;

        geomBufferInstData[i].posAngle = DirectX::XMFLOAT4(
            inst.position.x,
            inst.position.y,
            inst.position.z,
            angle);

        DirectX::XMFLOAT3 bbMin;
        DirectX::XMFLOAT3 bbMax;
        BuildWorldAABB(model, bbMin, bbMax);

        if (IsBoxInside(frustum, bbMin, bbMax))
            visibleIds.push_back(i);
    }

    m_SceneResources.UpdateGeomBufferInst(
        pDeviceContext,
        geomBufferInstData.data(),
        opaqueCount,
        visibleIds.empty() ? nullptr : visibleIds.data(),
        static_cast<UINT>(visibleIds.size()));

    pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    pDeviceContext->OMSetDepthStencilState(m_pOpaqueDepthState, 0);

    m_CubeObject.Render(
        pDeviceContext,
        m_SceneResources.GetSceneBuffer(),
        m_SceneResources.GetGeomBufferInst(),
        m_SceneResources.GetGeomBufferInstVis(),
        static_cast<UINT>(visibleIds.size()));

    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        DirectX::XMMatrixIdentity(),
        DirectX::XMFLOAT4(m_SceneResources.GetSkyRadius(), 0.0f, 0.0f, 0.0f),
        whiteColor);

    m_SkyboxObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    ID3D11ShaderResourceView* nullCubeResources[] = { nullptr, nullptr };
    pDeviceContext->PSSetShaderResources(0, 2, nullCubeResources);

    std::vector<TransparentInstance> transparentObjects;
    transparentObjects.push_back({
        DirectX::XMMatrixScaling(0.58f, 1.55f, 1.0f) *
        DirectX::XMMatrixTranslation(0.0f, 0.0f, 0.22f),
        DirectX::XMFLOAT4(1.0f, 0.12f, 0.12f, 0.40f),
        0.0f });

    transparentObjects.push_back({
        DirectX::XMMatrixScaling(0.58f, 1.55f, 1.0f) *
        DirectX::XMMatrixTranslation(0.0f, 0.0f, -0.08f),
        DirectX::XMFLOAT4(0.15f, 0.45f, 1.0f, 0.40f),
        0.0f });

    const DirectX::XMFLOAT3 cameraPos = m_SceneResources.GetCameraPosition();
    for (TransparentInstance& transparentObject : transparentObjects)
        transparentObject.sortKey = ComputeTransparentSortKey(transparentObject.model, cameraPos);

    std::sort(
        transparentObjects.begin(),
        transparentObjects.end(),
        [](const TransparentInstance& left, const TransparentInstance& right)
        {
            return left.sortKey > right.sortKey;
        });

    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    pDeviceContext->OMSetBlendState(m_pTransparentBlendState, blendFactor, 0xFFFFFFFF);

    for (const TransparentInstance& transparentObject : transparentObjects)
    {
        pDeviceContext->OMSetDepthStencilState(m_pTransparentDepthState, 0);

        m_SceneResources.UpdateGeomBuffer(
            pDeviceContext,
            transparentObject.model,
            zeroSize,
            transparentObject.color);

        m_TransparentQuadObject.Render(
            pDeviceContext,
            m_SceneResources.GetGeomBuffer(),
            m_SceneResources.GetSceneBuffer());
    }

    pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
    pDeviceContext->OMSetDepthStencilState(nullptr, 0);
    pDeviceContext->PSSetShaderResources(0, 2, nullCubeResources);

    RenderPostProcess();
    m_DeviceResources.Present();
}

void DxRenderer::OnResize(UINT width, UINT height)
{
    m_DeviceResources.OnResize(width, height);
}