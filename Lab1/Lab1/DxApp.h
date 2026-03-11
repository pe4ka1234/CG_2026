#pragma once

#include <Windows.h>
#include "DxRenderer.h"

class DxApp
{
public:
    bool Init(HWND hWnd, UINT width, UINT height);
    void Shutdown();

    void Render();
    void OnResize(UINT width, UINT height);

    void SetClearColor(float r, float g, float b, float a = 1.0f);

    void OnMouseDown(int x, int y);
    void OnMouseUp();
    void OnMouseMove(int x, int y, WPARAM buttons);

private:
    DxRenderer m_Renderer;

    bool m_IsMouseDragging = false;
    int m_LastMouseX = 0;
    int m_LastMouseY = 0;
};