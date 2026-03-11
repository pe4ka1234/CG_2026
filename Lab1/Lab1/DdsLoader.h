#pragma once

#include "DxCommon.h"

#include <vector>

struct TextureDesc
{
    UINT32 width = 0;
    UINT32 height = 0;
    UINT32 mipmapsCount = 0;
    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    std::vector<unsigned char> data;
};

UINT32 DivUp(UINT32 value, UINT32 divisor);
bool IsBlockCompressed(DXGI_FORMAT fmt);
UINT32 GetBytesPerBlock(DXGI_FORMAT fmt);
UINT32 GetBytesPerPixel(DXGI_FORMAT fmt);

bool LoadDDS(const wchar_t* path, TextureDesc& textureDesc);
void FillTextureSubresourceData(
    DXGI_FORMAT fmt,
    UINT32 width,
    UINT32 height,
    UINT32 mipLevels,
    const unsigned char* pSrcData,
    std::vector<D3D11_SUBRESOURCE_DATA>& outData);