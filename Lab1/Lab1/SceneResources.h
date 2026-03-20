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
        const DirectX::XMFLOAT4& color);

    ID3D11Buffer* GetSceneBuffer() const;
    ID3D11Buffer* GetGeomBuffer() const;
    float GetSkyRadius() const;
    DirectX::XMFLOAT3 GetCameraPosition() const;

private:
    bool CreateGeomBuffer(ID3D11Device* pDevice);
    bool CreateSceneBuffer(ID3D11Device* pDevice);
    void ClampCamera();

private:
    ID3D11Buffer* m_pGeomBuffer = nullptr;
    ID3D11Buffer* m_pSceneBuffer = nullptr;

    float m_CameraYaw = 0.0f;
    float m_CameraPitch = 0.0f;
    float m_CameraDistance = 3.0f;
    float m_SkyRadius = 1.0f;
    DirectX::XMFLOAT3 m_CameraPosition = DirectX::XMFLOAT3(0.0f, 0.0f, -3.0f);
};