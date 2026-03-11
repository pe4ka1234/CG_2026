#include "framework.h"
#include "DxRenderer.h"

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

    return true;
}

void DxRenderer::Shutdown()
{
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

    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        DirectX::XMMatrixIdentity(),
        DirectX::XMFLOAT4(m_SceneResources.GetSkyRadius(), 0.0f, 0.0f, 0.0f));
    m_SkyboxObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    m_SceneResources.UpdateGeomBuffer(
        pDeviceContext,
        DirectX::XMMatrixIdentity(),
        DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f));
    m_CubeObject.Render(
        pDeviceContext,
        m_SceneResources.GetGeomBuffer(),
        m_SceneResources.GetSceneBuffer());

    ID3D11ShaderResourceView* nullResources[] = { nullptr };
    pDeviceContext->PSSetShaderResources(0, 1, nullResources);

    m_DeviceResources.Present();
}

void DxRenderer::OnResize(UINT width, UINT height)
{
    m_DeviceResources.OnResize(width, height);
}