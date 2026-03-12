#include "framework.h"
#include "SceneResources.h"

#include "ShaderUtils.h"

#include <assert.h>
#include <cmath>

bool SceneResources::Init(ID3D11Device* pDevice)
{
    return CreateGeomBuffer(pDevice) && CreateSceneBuffer(pDevice);
}

bool SceneResources::CreateGeomBuffer(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(GeomBuffer);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pGeomBuffer);
    if (FAILED(hr) || !m_pGeomBuffer)
        return false;

    SetResourceName(m_pGeomBuffer, "GeomBuffer");
    return true;
}

bool SceneResources::CreateSceneBuffer(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(SceneBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pSceneBuffer);
    if (FAILED(hr) || !m_pSceneBuffer)
        return false;

    SetResourceName(m_pSceneBuffer, "SceneBuffer");
    return true;
}

void SceneResources::RotateCamera(float deltaYaw, float deltaPitch)
{
    m_CameraYaw += deltaYaw;
    m_CameraPitch += deltaPitch;
    ClampCamera();
}

void SceneResources::ClampCamera()
{
    const float pitchLimit = 1.35f;
    if (m_CameraPitch > pitchLimit)
        m_CameraPitch = pitchLimit;
    if (m_CameraPitch < -pitchLimit)
        m_CameraPitch = -pitchLimit;
}

bool SceneResources::UpdateSceneBuffer(ID3D11DeviceContext* pDeviceContext, UINT width, UINT height)
{
    if (!m_pSceneBuffer || !pDeviceContext || width == 0 || height == 0)
        return false;

    constexpr float PI = 3.14159265358979323846f;

    const DirectX::XMMATRIX cameraRotation = DirectX::XMMatrixRotationRollPitchYaw(
        m_CameraPitch,
        m_CameraYaw,
        0.0f);

    const DirectX::XMMATRIX cameraTransform = DirectX::XMMatrixMultiply(
        DirectX::XMMatrixTranslation(0.0f, 0.0f, -m_CameraDistance),
        cameraRotation);

    const DirectX::XMMATRIX view = DirectX::XMMatrixInverse(nullptr, cameraTransform);

    const float f = 100.0f;
    const float n = 0.1f;
    const float fov = PI / 3.0f;
    const float aspectRatio = static_cast<float>(height) / static_cast<float>(width);
    const float nearWidth = tanf(fov / 2.0f) * 2.0f * n;
    const float nearHeight = nearWidth * aspectRatio;

    const DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveLH(
        nearWidth,
        nearHeight,
        n,
        f);

    const DirectX::XMVECTOR cameraPosVec = DirectX::XMVector3TransformCoord(
        DirectX::XMVectorZero(),
        cameraTransform);

    DirectX::XMFLOAT3 cameraPos{};
    DirectX::XMStoreFloat3(&cameraPos, cameraPosVec);

    D3D11_MAPPED_SUBRESOURCE subresource{};
    const HRESULT hr = pDeviceContext->Map(
        m_pSceneBuffer,
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &subresource);

    assert(SUCCEEDED(hr));
    if (FAILED(hr))
        return false;

    SceneBuffer& sceneBuffer = *reinterpret_cast<SceneBuffer*>(subresource.pData);
    DirectX::XMStoreFloat4x4(&sceneBuffer.vp, DirectX::XMMatrixMultiply(view, proj));
    sceneBuffer.cameraPos = DirectX::XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
    pDeviceContext->Unmap(m_pSceneBuffer, 0);

    m_SkyRadius = sqrtf(
        n * n +
        (nearWidth * 0.5f) * (nearWidth * 0.5f) +
        (nearHeight * 0.5f) * (nearHeight * 0.5f)) * 1.05f;

    return true;
}

void SceneResources::UpdateGeomBuffer(
    ID3D11DeviceContext* pDeviceContext,
    const DirectX::XMMATRIX& model,
    const DirectX::XMFLOAT4& size)
{
    if (!m_pGeomBuffer || !pDeviceContext)
        return;

    GeomBuffer geomBuffer{};
    DirectX::XMStoreFloat4x4(&geomBuffer.model, model);
    geomBuffer.size = size;
    pDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &geomBuffer, 0, 0);
}

ID3D11Buffer* SceneResources::GetSceneBuffer() const
{
    return m_pSceneBuffer;
}

ID3D11Buffer* SceneResources::GetGeomBuffer() const
{
    return m_pGeomBuffer;
}

float SceneResources::GetSkyRadius() const
{
    return m_SkyRadius;
}

void SceneResources::Shutdown()
{
    SAFE_RELEASE(m_pSceneBuffer);
    SAFE_RELEASE(m_pGeomBuffer);
}