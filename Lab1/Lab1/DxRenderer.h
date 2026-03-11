#pragma once

#include "DxCommon.h"

#include "CubeObject.h"
#include "DeviceResources.h"
#include "SceneResources.h"
#include "SkyboxObject.h"

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
    float m_ClearColor[4] = { 0.60f, 0.80f, 1.00f, 1.0f };

    DeviceResources m_DeviceResources;
    SceneResources m_SceneResources;
    CubeObject m_CubeObject;
    SkyboxObject m_SkyboxObject;
};