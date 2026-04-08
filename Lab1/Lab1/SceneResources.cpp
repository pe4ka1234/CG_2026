#include "framework.h"
#include "SceneResources.h"

#include "ShaderUtils.h"

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <cstring>

namespace
{
    DirectX::XMFLOAT3 TransformClipToWorld(
        const DirectX::XMMATRIX& invVP,
        float x,
        float y,
        float z)
    {
        DirectX::XMVECTOR p = DirectX::XMVectorSet(x, y, z, 1.0f);
        p = DirectX::XMVector4Transform(p, invVP);

        const float w = DirectX::XMVectorGetW(p);
        p = DirectX::XMVectorScale(p, 1.0f / w);

        DirectX::XMFLOAT3 result;
        DirectX::XMStoreFloat3(&result, p);
        return result;
    }

    DirectX::XMFLOAT4 BuildPlane(
        const DirectX::XMFLOAT3& p0,
        const DirectX::XMFLOAT3& p1,
        const DirectX::XMFLOAT3& p2,
        const DirectX::XMFLOAT3& p3)
    {
        const DirectX::XMVECTOR v0 = DirectX::XMLoadFloat3(&p0);
        const DirectX::XMVECTOR v1 = DirectX::XMLoadFloat3(&p1);
        const DirectX::XMVECTOR v2 = DirectX::XMLoadFloat3(&p2);
        const DirectX::XMVECTOR v3 = DirectX::XMLoadFloat3(&p3);

        const DirectX::XMVECTOR edge1 = DirectX::XMVectorSubtract(v1, v0);
        const DirectX::XMVECTOR edge2 = DirectX::XMVectorSubtract(v3, v0);

        DirectX::XMVECTOR norm = DirectX::XMVector3Cross(edge1, edge2);
        norm = DirectX::XMVector3Normalize(norm);

        const DirectX::XMVECTOR sum01 = DirectX::XMVectorAdd(v0, v1);
        const DirectX::XMVECTOR sum23 = DirectX::XMVectorAdd(v2, v3);
        const DirectX::XMVECTOR sum = DirectX::XMVectorAdd(sum01, sum23);
        const DirectX::XMVECTOR pos = DirectX::XMVectorScale(sum, 0.25f);

        const float d = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(pos, norm));

        DirectX::XMFLOAT3 norm3;
        DirectX::XMStoreFloat3(&norm3, norm);

        return DirectX::XMFLOAT4(norm3.x, norm3.y, norm3.z, d);
    }
}

bool SceneResources::Init(ID3D11Device* pDevice)
{
    return CreateGeomBuffer(pDevice) &&
        CreateGeomBufferInst(pDevice) &&
        CreateGeomBufferInstVis(pDevice) &&
        CreateSceneBuffer(pDevice) &&
        CreateCullParams(pDevice) &&
        CreateIndirectArgs(pDevice) &&
        CreateGeomBufferInstVisGPU(pDevice);
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

bool SceneResources::CreateGeomBufferInst(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(GeomBufferInst);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInst);
    if (FAILED(hr) || !m_pGeomBufferInst)
        return false;

    SetResourceName(m_pGeomBufferInst, "GeomBufferInst");
    return true;
}

bool SceneResources::CreateGeomBufferInstVis(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(GeomBufferInstVis);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInstVis);
    if (FAILED(hr) || !m_pGeomBufferInstVis)
        return false;

    SetResourceName(m_pGeomBufferInstVis, "GeomBufferInstVis");
    return true;
}

bool SceneResources::CreateSceneBuffer(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(SceneBuffer);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pSceneBuffer);
    if (FAILED(hr) || !m_pSceneBuffer)
        return false;

    SetResourceName(m_pSceneBuffer, "SceneBuffer");
    return true;
}

bool SceneResources::CreateCullParams(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(CullParams);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pCullParams);
    if (FAILED(hr) || !m_pCullParams)
        return false;

    SetResourceName(m_pCullParams, "CullParams");
    return true;
}

bool SceneResources::CreateIndirectArgs(ID3D11Device* pDevice)
{
    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof(UINT);

        HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pIndirectArgsSrc);
        if (FAILED(hr) || !m_pIndirectArgsSrc)
            return false;

        hr = pDevice->CreateUnorderedAccessView(m_pIndirectArgsSrc, nullptr, &m_pIndirectArgsUAV);
        if (FAILED(hr) || !m_pIndirectArgsUAV)
            return false;

        SetResourceName(m_pIndirectArgsSrc, "IndirectArgsSrc");
        SetResourceName(m_pIndirectArgsUAV, "IndirectArgsUAV");
    }

    {
        D3D11_BUFFER_DESC desc{};
        desc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;
        desc.StructureByteStride = 0;

        const HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pIndirectArgs);
        if (FAILED(hr) || !m_pIndirectArgs)
            return false;

        SetResourceName(m_pIndirectArgs, "IndirectArgs");
    }

    return true;
}

bool SceneResources::CreateGeomBufferInstVisGPU(ID3D11Device* pDevice)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = sizeof(GeomBufferInstVis);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(DirectX::XMUINT4);

    HRESULT hr = pDevice->CreateBuffer(&desc, nullptr, &m_pGeomBufferInstVisGPU);
    if (FAILED(hr) || !m_pGeomBufferInstVisGPU)
        return false;

    hr = pDevice->CreateUnorderedAccessView(m_pGeomBufferInstVisGPU, nullptr, &m_pGeomBufferInstVisGPU_UAV);
    if (FAILED(hr) || !m_pGeomBufferInstVisGPU_UAV)
        return false;

    SetResourceName(m_pGeomBufferInstVisGPU, "GeomBufferInstVisGPU");
    SetResourceName(m_pGeomBufferInstVisGPU_UAV, "GeomBufferInstVisGPU_UAV");
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

void SceneResources::BuildFrustum(const DirectX::XMMATRIX& vp)
{
    const DirectX::XMMATRIX invVP = DirectX::XMMatrixInverse(nullptr, vp);

    const DirectX::XMFLOAT3 ntl = TransformClipToWorld(invVP, -1.0f, 1.0f, 1.0f);
    const DirectX::XMFLOAT3 ntr = TransformClipToWorld(invVP, 1.0f, 1.0f, 1.0f);
    const DirectX::XMFLOAT3 nbr = TransformClipToWorld(invVP, 1.0f, -1.0f, 1.0f);
    const DirectX::XMFLOAT3 nbl = TransformClipToWorld(invVP, -1.0f, -1.0f, 1.0f);

    const DirectX::XMFLOAT3 ftl = TransformClipToWorld(invVP, -1.0f, 1.0f, 0.0f);
    const DirectX::XMFLOAT3 ftr = TransformClipToWorld(invVP, 1.0f, 1.0f, 0.0f);
    const DirectX::XMFLOAT3 fbr = TransformClipToWorld(invVP, 1.0f, -1.0f, 0.0f);
    const DirectX::XMFLOAT3 fbl = TransformClipToWorld(invVP, -1.0f, -1.0f, 0.0f);

    m_Frustum[0] = BuildPlane(ntl, nbl, nbr, ntr); // near
    m_Frustum[1] = BuildPlane(ftl, ftr, fbr, fbl); // far
    m_Frustum[2] = BuildPlane(ntl, ftl, fbl, nbl); // left
    m_Frustum[3] = BuildPlane(ntr, nbr, fbr, ftr); // right
    m_Frustum[4] = BuildPlane(ntl, ntr, ftr, ftl); // top
    m_Frustum[5] = BuildPlane(nbl, fbl, fbr, nbr); // bottom
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

    const float farWidth = tanf(fov / 2.0f) * 2.0f * f;
    const float farHeight = farWidth * aspectRatio;

    const DirectX::XMMATRIX proj = DirectX::XMMatrixPerspectiveLH(
        farWidth,
        farHeight,
        f,
        n);

    const float nearWidth = tanf(fov / 2.0f) * 2.0f * n;
    const float nearHeight = nearWidth * aspectRatio;

    const DirectX::XMVECTOR cameraPosVec = DirectX::XMVector3TransformCoord(
        DirectX::XMVectorZero(),
        cameraTransform);

    DirectX::XMStoreFloat3(&m_CameraPosition, cameraPosVec);

    const DirectX::XMMATRIX vp = DirectX::XMMatrixMultiply(view, proj);
    BuildFrustum(vp);

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

    SceneBuffer sceneBuffer{};
    DirectX::XMStoreFloat4x4(&sceneBuffer.vp, vp);
    sceneBuffer.cameraPos = DirectX::XMFLOAT4(
        m_CameraPosition.x,
        m_CameraPosition.y,
        m_CameraPosition.z,
        1.0f);

    sceneBuffer.lightCount.x = 2;
    sceneBuffer.lightCount.y = 1;
    sceneBuffer.lightCount.z = 0;
    sceneBuffer.lightCount.w = 1;

    sceneBuffer.postProcess.x = 1;
    sceneBuffer.postProcess.y = 0;
    sceneBuffer.postProcess.z = 0;
    sceneBuffer.postProcess.w = 0;

    sceneBuffer.ambientColor = DirectX::XMFLOAT4(0.20f, 0.20f, 0.22f, 0.0f);

    sceneBuffer.lights[0].pos = DirectX::XMFLOAT4(0.95f, 0.32f, -1.05f, 1.0f);
    sceneBuffer.lights[0].color = DirectX::XMFLOAT4(1.95f, 1.20f, 0.72f, 0.0f);

    sceneBuffer.lights[1].pos = DirectX::XMFLOAT4(-1.10f, 0.95f, -0.35f, 1.0f);
    sceneBuffer.lights[1].color = DirectX::XMFLOAT4(0.46f, 0.62f, 1.18f, 0.0f);

    for (int i = 0; i < 6; ++i)
        sceneBuffer.frustum[i] = m_Frustum[i];

    std::memcpy(subresource.pData, &sceneBuffer, sizeof(sceneBuffer));
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
    const DirectX::XMFLOAT4& size,
    const DirectX::XMFLOAT4& color,
    float shininess)
{
    if (!m_pGeomBuffer || !pDeviceContext)
        return;

    GeomBuffer geomBuffer{};
    DirectX::XMStoreFloat4x4(&geomBuffer.model, model);
    geomBuffer.size = size;
    geomBuffer.color = color;

    const DirectX::XMMATRIX normalModel = DirectX::XMMatrixTranspose(
        DirectX::XMMatrixInverse(nullptr, model));
    DirectX::XMStoreFloat4x4(&geomBuffer.normalModel, normalModel);

    geomBuffer.materialParams = DirectX::XMFLOAT4(
        shininess,
        0.0f,
        0.0f,
        0.0f);

    pDeviceContext->UpdateSubresource(m_pGeomBuffer, 0, nullptr, &geomBuffer, 0, 0);
}

void SceneResources::UpdateGeomBufferInst(
    ID3D11DeviceContext* pDeviceContext,
    const GeomBufferInstData* pGeomBuffers,
    UINT geomBufferCount,
    const UINT* pVisibleIds,
    UINT visibleCount)
{
    if (!m_pGeomBufferInst || !m_pGeomBufferInstVis || !pDeviceContext)
        return;

    if (geomBufferCount > MaxInst)
        geomBufferCount = MaxInst;
    if (visibleCount > MaxInst)
        visibleCount = MaxInst;

    GeomBufferInst geomBufferInst{};
    for (UINT i = 0; i < geomBufferCount; ++i)
        geomBufferInst.geomBuffer[i] = pGeomBuffers[i];

    pDeviceContext->UpdateSubresource(m_pGeomBufferInst, 0, nullptr, &geomBufferInst, 0, 0);

    GeomBufferInstVis geomBufferInstVis{};
    if (pVisibleIds)
    {
        for (UINT i = 0; i < visibleCount; ++i)
        {
            geomBufferInstVis.ids[i].x = pVisibleIds[i];
            geomBufferInstVis.ids[i].y = 0;
            geomBufferInstVis.ids[i].z = 0;
            geomBufferInstVis.ids[i].w = 0;
        }
    }

    pDeviceContext->UpdateSubresource(m_pGeomBufferInstVis, 0, nullptr, &geomBufferInstVis, 0, 0);
}

void SceneResources::UpdateCullParams(
    ID3D11DeviceContext* pDeviceContext,
    const DirectX::XMFLOAT4* pBBMin,
    const DirectX::XMFLOAT4* pBBMax,
    UINT shapeCount)
{
    if (!m_pCullParams || !pDeviceContext)
        return;

    if (shapeCount > MaxInst)
        shapeCount = MaxInst;

    CullParams cullParams{};
    cullParams.numShapes.x = shapeCount;
    cullParams.numShapes.y = 0;
    cullParams.numShapes.z = 0;
    cullParams.numShapes.w = 0;

    if (pBBMin && pBBMax)
    {
        for (UINT i = 0; i < shapeCount; ++i)
        {
            cullParams.bbMin[i] = pBBMin[i];
            cullParams.bbMax[i] = pBBMax[i];
        }
    }

    pDeviceContext->UpdateSubresource(m_pCullParams, 0, nullptr, &cullParams, 0, 0);
}

ID3D11Buffer* SceneResources::GetSceneBuffer() const
{
    return m_pSceneBuffer;
}

ID3D11Buffer* SceneResources::GetGeomBuffer() const
{
    return m_pGeomBuffer;
}

ID3D11Buffer* SceneResources::GetGeomBufferInst() const
{
    return m_pGeomBufferInst;
}

ID3D11Buffer* SceneResources::GetGeomBufferInstVis() const
{
    return m_pGeomBufferInstVis;
}

ID3D11Buffer* SceneResources::GetCullParams() const
{
    return m_pCullParams;
}

ID3D11Buffer* SceneResources::GetIndirectArgsSrc() const
{
    return m_pIndirectArgsSrc;
}

ID3D11UnorderedAccessView* SceneResources::GetIndirectArgsUAV() const
{
    return m_pIndirectArgsUAV;
}

ID3D11Buffer* SceneResources::GetIndirectArgs() const
{
    return m_pIndirectArgs;
}

ID3D11Buffer* SceneResources::GetGeomBufferInstVisGPU() const
{
    return m_pGeomBufferInstVisGPU;
}

ID3D11UnorderedAccessView* SceneResources::GetGeomBufferInstVisGPU_UAV() const
{
    return m_pGeomBufferInstVisGPU_UAV;
}

float SceneResources::GetSkyRadius() const
{
    return m_SkyRadius;
}

DirectX::XMFLOAT3 SceneResources::GetCameraPosition() const
{
    return m_CameraPosition;
}

const DirectX::XMFLOAT4* SceneResources::GetFrustum() const
{
    return m_Frustum;
}

void SceneResources::Shutdown()
{
    SAFE_RELEASE(m_pGeomBufferInstVisGPU_UAV);
    SAFE_RELEASE(m_pGeomBufferInstVisGPU);

    SAFE_RELEASE(m_pIndirectArgs);
    SAFE_RELEASE(m_pIndirectArgsUAV);
    SAFE_RELEASE(m_pIndirectArgsSrc);

    SAFE_RELEASE(m_pCullParams);
    SAFE_RELEASE(m_pSceneBuffer);
    SAFE_RELEASE(m_pGeomBufferInstVis);
    SAFE_RELEASE(m_pGeomBufferInst);
    SAFE_RELEASE(m_pGeomBuffer);
}