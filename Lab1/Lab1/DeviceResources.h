#pragma once

#include "DxCommon.h"

class DeviceResources
{
public:
    bool Init(HWND hWnd, UINT width, UINT height);
    void Shutdown();

    void OnResize(UINT width, UINT height);
    void BeginFrame(const float clearColor[4]);
    void Present();

    bool IsReady() const;

    ID3D11Device* GetDevice() const;
    ID3D11DeviceContext* GetContext() const;
    UINT GetWidth() const;
    UINT GetHeight() const;

private:
    bool CreateDeviceAndSwapChain();
    bool CreateBackBufferRTV();
    void ReleaseBackBufferRTV();

    bool CreateDepthBuffer();
    void ReleaseDepthBuffer();

private:
    HWND m_hWnd = nullptr;
    UINT m_Width = 1;
    UINT m_Height = 1;

    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;

    ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
    ID3D11Texture2D* m_pDepthTexture = nullptr;
    ID3D11DepthStencilView* m_pDepthStencilView = nullptr;
};