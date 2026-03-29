#include "framework.h"
#include "DxRenderer.h"

#include <algorithm>
#include <vector>

namespace
{
    struct TransparentInstance
    {
        DirectX::XMMATRIX model;
        DirectX::XMFLOAT4 color;
        float sortKey = 0.0f;
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

void DxRenderer::Shutdown()
{
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

    const DirectX::XMMATRIX cubeModelA =
        DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) *
        DirectX::XMMatrixRotationY(0.18f) *
        DirectX::XMMatrixTranslation(-0.95f, 0.0f, 0.18f);

    const DirectX::XMMATRIX cubeModelB =
        DirectX::XMMatrixScaling(0.85f, 0.85f, 0.85f) *
        DirectX::XMMatrixRotationY(-0.22f) *
        DirectX::XMMatrixTranslation(0.95f, 0.02f, -0.10f);

    pDeviceContext->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

    pDeviceContext->OMSetDepthStencilState(m_pOpaqueDepthState, 0);
    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        cubeModelA,
        zeroSize,
        whiteColor);
    m_CubeObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    pDeviceContext->OMSetDepthStencilState(m_pOpaqueDepthState, 0);
    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        cubeModelB,
        zeroSize,
        whiteColor);
    m_CubeObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        DirectX::XMMatrixIdentity(),
        DirectX::XMFLOAT4(m_SceneResources.GetSkyRadius(), 0.0f, 0.0f, 0.0f),
        whiteColor);
    m_SkyboxObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    ID3D11ShaderResourceView* nullResources[] = { nullptr, nullptr };
    pDeviceContext->PSSetShaderResources(0, 2, nullResources);

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
    pDeviceContext->PSSetShaderResources(0, 2, nullResources);

    m_DeviceResources.Present();
}

void DxRenderer::OnResize(UINT width, UINT height)
{
    m_DeviceResources.OnResize(width, height);
}