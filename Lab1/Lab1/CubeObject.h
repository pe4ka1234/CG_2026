#pragma once

#include "DxCommon.h"
#include "RenderTypes.h"

class CubeObject
{
public:
    bool Init(ID3D11Device* pDevice);
    void Shutdown();

    bool IsReady() const;
    void Render(
        ID3D11DeviceContext* pDeviceContext,
        ID3D11Buffer* pSceneBuffer,
        ID3D11Buffer* pGeomBufferInst,
        ID3D11Buffer* pGeomBufferInstVis,
        UINT instanceCount);

private:
    struct Geometry
    {
        ID3D11Buffer* pVertexBuffer = nullptr;
        ID3D11Buffer* pIndexBuffer = nullptr;
        UINT indexCount = 0;
    };

    struct Material
    {
        ID3D11VertexShader* pVertexShader = nullptr;
        ID3D11PixelShader* pPixelShader = nullptr;
        ID3D11InputLayout* pInputLayout = nullptr;

        ID3D11Texture2D* pColorTextureArray = nullptr;
        ID3D11ShaderResourceView* pColorTextureArrayView = nullptr;

        ID3D11Texture2D* pNormalTexture = nullptr;
        ID3D11ShaderResourceView* pNormalTextureView = nullptr;

        ID3D11SamplerState* pSampler = nullptr;
    };

private:
    bool CreateGeometry(ID3D11Device* pDevice);
    bool CreateShaders(ID3D11Device* pDevice);
    bool CreateTextures(ID3D11Device* pDevice);
    bool CreateSampler(ID3D11Device* pDevice);

private:
    Geometry m_Geometry;
    Material m_Material;
};