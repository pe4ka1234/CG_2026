#pragma once

#include "DxCommon.h"

#include "CubeObject.h"
#include "DeviceResources.h"
#include "SceneResources.h"
#include "SkyboxObject.h"
#include "TransparentQuadObject.h"

class DxRenderer
{
public:
    bool Init(HWND hWnd, UINT width, UINT height);
    void Shutdown();

    void Render();
    void OnResize(UINT width, UINT height);

    void SetClearColor(float r, float g, float b, float a = 1.0f);
    void RotateCamera(float deltaYaw, float deltaPitch);

private:
    bool CreateStates();
    void ReleaseStates();

    bool CreatePostProcessResources();
    void ReleasePostProcessResources();
    void RenderPostProcess();

private:
    float m_ClearColor[4] = { 0.60f, 0.80f, 1.00f, 1.0f };

    DeviceResources m_DeviceResources;
    SceneResources m_SceneResources;
    CubeObject m_CubeObject;
    SkyboxObject m_SkyboxObject;
    TransparentQuadObject m_TransparentQuadObject;

    ID3D11DepthStencilState* m_pOpaqueDepthState = nullptr;
    ID3D11DepthStencilState* m_pTransparentDepthState = nullptr;
    ID3D11BlendState* m_pTransparentBlendState = nullptr;

    ID3D11VertexShader* m_pSepiaVertexShader = nullptr;
    ID3D11PixelShader* m_pSepiaPixelShader = nullptr;
    ID3D11SamplerState* m_pPostProcessSampler = nullptr;
};