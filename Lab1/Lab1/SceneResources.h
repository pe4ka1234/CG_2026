#pragma once

#include "DxCommon.h"
#include "RenderTypes.h"

class SceneResources
{
public:
    bool Init(ID3D11Device* pDevice);
    void Shutdown();

    void RotateCamera(float deltaYaw, float deltaPitch);

    bool UpdateSceneBuffer(ID3D11DeviceContext* pDeviceContext, UINT width, UINT height);

    void UpdateGeomBuffer(
        ID3D11DeviceContext* pDeviceContext,
        const DirectX::XMMATRIX& model,
        const DirectX::XMFLOAT4& size,
        const DirectX::XMFLOAT4& color,
        float shininess = 48.0f);

    void UpdateGeomBufferInst(
        ID3D11DeviceContext* pDeviceContext,
        const GeomBufferInstData* pGeomBuffers,
        UINT geomBufferCount,
        const UINT* pVisibleIds,
        UINT visibleCount);

    void UpdateCullParams(
        ID3D11DeviceContext* pDeviceContext,
        const DirectX::XMFLOAT4* pBBMin,
        const DirectX::XMFLOAT4* pBBMax,
        UINT shapeCount);

    ID3D11Buffer* GetSceneBuffer() const;
    ID3D11Buffer* GetGeomBuffer() const;
    ID3D11Buffer* GetGeomBufferInst() const;
    ID3D11Buffer* GetGeomBufferInstVis() const;
    ID3D11Buffer* GetCullParams() const;
    ID3D11Buffer* GetIndirectArgsSrc() const;
    ID3D11UnorderedAccessView* GetIndirectArgsUAV() const;
    ID3D11Buffer* GetIndirectArgs() const;
    ID3D11Buffer* GetGeomBufferInstVisGPU() const;
    ID3D11UnorderedAccessView* GetGeomBufferInstVisGPU_UAV() const;

    float GetSkyRadius() const;
    DirectX::XMFLOAT3 GetCameraPosition() const;
    const DirectX::XMFLOAT4* GetFrustum() const;

private:
    bool CreateGeomBuffer(ID3D11Device* pDevice);
    bool CreateGeomBufferInst(ID3D11Device* pDevice);
    bool CreateGeomBufferInstVis(ID3D11Device* pDevice);
    bool CreateSceneBuffer(ID3D11Device* pDevice);
    bool CreateCullParams(ID3D11Device* pDevice);
    bool CreateIndirectArgs(ID3D11Device* pDevice);
    bool CreateGeomBufferInstVisGPU(ID3D11Device* pDevice);

    void ClampCamera();
    void BuildFrustum(const DirectX::XMMATRIX& vp);

private:
    ID3D11Buffer* m_pGeomBuffer = nullptr;
    ID3D11Buffer* m_pGeomBufferInst = nullptr;
    ID3D11Buffer* m_pGeomBufferInstVis = nullptr;
    ID3D11Buffer* m_pSceneBuffer = nullptr;
    ID3D11Buffer* m_pCullParams = nullptr;

    ID3D11Buffer* m_pIndirectArgsSrc = nullptr;
    ID3D11UnorderedAccessView* m_pIndirectArgsUAV = nullptr;
    ID3D11Buffer* m_pIndirectArgs = nullptr;

    ID3D11Buffer* m_pGeomBufferInstVisGPU = nullptr;
    ID3D11UnorderedAccessView* m_pGeomBufferInstVisGPU_UAV = nullptr;

    float m_CameraYaw = 0.0f;
    float m_CameraPitch = 0.0f;
    float m_CameraDistance = 3.0f;
    float m_SkyRadius = 1.0f;

    DirectX::XMFLOAT3 m_CameraPosition = DirectX::XMFLOAT3(0.0f, 0.0f, -3.0f);
    DirectX::XMFLOAT4 m_Frustum[6] = {};
};