#pragma once

#include "DxCommon.h"

constexpr UINT MaxInst = 100;

struct TextureVertex
{
    float x, y, z;
    float u, v;
};

struct TextureTangentVertex
{
    float x, y, z;
    float tx, ty, tz;
    float nx, ny, nz;
    float u, v;
};

struct SkyVertex
{
    float x, y, z;
};

struct Light
{
    DirectX::XMFLOAT4 pos = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    DirectX::XMFLOAT4 color = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
};

struct GeomBuffer
{
    DirectX::XMFLOAT4X4 model;
    DirectX::XMFLOAT4 size;
    DirectX::XMFLOAT4 color;
    DirectX::XMFLOAT4X4 normalModel;
    DirectX::XMFLOAT4 materialParams;
};

struct GeomBufferInstData
{
    DirectX::XMFLOAT4X4 model;
    DirectX::XMFLOAT4X4 norm;
    DirectX::XMFLOAT4 shineSpeedTexIdNM; 
    DirectX::XMFLOAT4 posAngle;          
};

struct GeomBufferInst
{
    GeomBufferInstData geomBuffer[MaxInst];
};

struct GeomBufferInstVis
{
    DirectX::XMUINT4 ids[MaxInst];
};

struct SceneBuffer
{
    DirectX::XMFLOAT4X4 vp;
    DirectX::XMFLOAT4 cameraPos;
    DirectX::XMINT4 lightCount;
    Light lights[10];
    DirectX::XMFLOAT4 ambientColor;
};