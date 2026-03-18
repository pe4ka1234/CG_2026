#pragma once

#include "DxCommon.h"

struct TextureVertex
{
    float x, y, z;
    float u, v;
};

struct SkyVertex
{
    float x, y, z;
};

struct GeomBuffer
{
    DirectX::XMFLOAT4X4 model;
    DirectX::XMFLOAT4 size;
    DirectX::XMFLOAT4 color;
};

struct SceneBuffer
{
    DirectX::XMFLOAT4X4 vp;
    DirectX::XMFLOAT4 cameraPos;
};