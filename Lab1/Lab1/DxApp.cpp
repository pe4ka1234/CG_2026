#include "framework.h"
#include "DxApp.h"

bool DxApp::Init(HWND hWnd, UINT width, UINT height)
{
    return m_Renderer.Init(hWnd, width, height);
}

void DxApp::Shutdown()
{
    m_Renderer.Shutdown();
}

void DxApp::Render()
{
    m_Renderer.Render();
}

void DxApp::OnResize(UINT width, UINT height)
{
    m_Renderer.OnResize(width, height);
}

void DxApp::SetClearColor(float r, float g, float b, float a)
{
    m_Renderer.SetClearColor(r, g, b, a);
}

void DxApp::OnMouseDown(int x, int y)
{
    m_IsMouseDragging = true;
    m_LastMouseX = x;
    m_LastMouseY = y;
}

void DxApp::OnMouseUp()
{
    m_IsMouseDragging = false;
}

void DxApp::OnMouseMove(int x, int y, WPARAM buttons)
{
    if (!m_IsMouseDragging)
        return;

    if ((buttons & MK_LBUTTON) == 0)
    {
        m_IsMouseDragging = false;
        return;
    }

    const int dx = x - m_LastMouseX;
    const int dy = y - m_LastMouseY;

    m_LastMouseX = x;
    m_LastMouseY = y;

    const float mouseSensitivity = 0.0032f;
    m_Renderer.RotateCamera(dx * mouseSensitivity, dy * mouseSensitivity);
}