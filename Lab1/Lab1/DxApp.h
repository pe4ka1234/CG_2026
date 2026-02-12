#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) do { if ((p) != nullptr) { (p)->Release(); (p) = nullptr; } } while (0)
#endif

class DxApp
{
public:
    bool Init(HWND hWnd, UINT width, UINT height);
    void Shutdown();

    void Render();
    void OnResize(UINT width, UINT height);

    void SetClearColor(float r, float g, float b, float a = 1.0f);

private:
    bool CreateBackBufferRTV();
    void ReleaseBackBufferRTV();

private:
    HWND m_hWnd = nullptr;
    UINT m_Width = 1;
    UINT m_Height = 1;

    float m_ClearColor[4] = { 0.60f, 0.80f, 1.00f, 1.0f }; 

    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pDeviceContext = nullptr;
    IDXGISwapChain* m_pSwapChain = nullptr;
    ID3D11RenderTargetView* m_pBackBufferRTV = nullptr;
};
