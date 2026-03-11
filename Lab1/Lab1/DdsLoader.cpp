#include "framework.h"
#include "DdsLoader.h"
#include "ShaderUtils.h"

#include <algorithm>

static UINT32 MakeFourCC(char c0, char c1, char c2, char c3)
{
    return UINT32(static_cast<unsigned char>(c0)) |
        (UINT32(static_cast<unsigned char>(c1)) << 8) |
        (UINT32(static_cast<unsigned char>(c2)) << 16) |
        (UINT32(static_cast<unsigned char>(c3)) << 24);
}

#pragma pack(push, 1)
struct DDS_PIXELFORMAT
{
    UINT32 size;
    UINT32 flags;
    UINT32 fourCC;
    UINT32 rgbBitCount;
    UINT32 rBitMask;
    UINT32 gBitMask;
    UINT32 bBitMask;
    UINT32 aBitMask;
};

struct DDS_HEADER
{
    UINT32 size;
    UINT32 flags;
    UINT32 height;
    UINT32 width;
    UINT32 pitchOrLinearSize;
    UINT32 depth;
    UINT32 mipMapCount;
    UINT32 reserved1[11];
    DDS_PIXELFORMAT ddspf;
    UINT32 caps;
    UINT32 caps2;
    UINT32 caps3;
    UINT32 caps4;
    UINT32 reserved2;
};
#pragma pack(pop)

UINT32 DivUp(UINT32 value, UINT32 divisor)
{
    return (value + divisor - 1u) / divisor;
}

bool IsBlockCompressed(DXGI_FORMAT fmt)
{
    return fmt == DXGI_FORMAT_BC1_UNORM ||
        fmt == DXGI_FORMAT_BC2_UNORM ||
        fmt == DXGI_FORMAT_BC3_UNORM;
}

UINT32 GetBytesPerBlock(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_UNORM:
        return 8;
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC3_UNORM:
        return 16;
    default:
        return 0;
    }
}

UINT32 GetBytesPerPixel(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return 4;
    default:
        return 0;
    }
}

bool LoadDDS(const wchar_t* path, TextureDesc& textureDesc)
{
    textureDesc = {};

    std::vector<unsigned char> fileData;
    if (!ReadAllBytes(path, fileData))
        return false;

    const UINT32 DDS_MAGIC = MakeFourCC('D', 'D', 'S', ' ');
    if (fileData.size() < sizeof(UINT32) + sizeof(DDS_HEADER))
        return false;

    const UINT32 magic = *reinterpret_cast<const UINT32*>(fileData.data());
    if (magic != DDS_MAGIC)
        return false;

    const DDS_HEADER* header = reinterpret_cast<const DDS_HEADER*>(fileData.data() + sizeof(UINT32));
    if (header->size != 124 || header->ddspf.size != 32)
        return false;

    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;

    if (header->ddspf.fourCC == MakeFourCC('D', 'X', 'T', '1'))
        fmt = DXGI_FORMAT_BC1_UNORM;
    else if (header->ddspf.fourCC == MakeFourCC('D', 'X', 'T', '3'))
        fmt = DXGI_FORMAT_BC2_UNORM;
    else if (header->ddspf.fourCC == MakeFourCC('D', 'X', 'T', '5'))
        fmt = DXGI_FORMAT_BC3_UNORM;
    else if (header->ddspf.fourCC == MakeFourCC('D', 'X', '1', '0'))
        return false;
    else if (header->ddspf.rgbBitCount == 32 &&
        header->ddspf.rBitMask == 0x000000ff &&
        header->ddspf.gBitMask == 0x0000ff00 &&
        header->ddspf.bBitMask == 0x00ff0000 &&
        header->ddspf.aBitMask == 0xff000000)
        fmt = DXGI_FORMAT_R8G8B8A8_UNORM;
    else if (header->ddspf.rgbBitCount == 32 &&
        header->ddspf.rBitMask == 0x00ff0000 &&
        header->ddspf.gBitMask == 0x0000ff00 &&
        header->ddspf.bBitMask == 0x000000ff &&
        header->ddspf.aBitMask == 0xff000000)
        fmt = DXGI_FORMAT_B8G8R8A8_UNORM;
    else
        return false;

    const size_t dataOffset = sizeof(UINT32) + sizeof(DDS_HEADER);
    if (fileData.size() <= dataOffset)
        return false;

    textureDesc.width = header->width;
    textureDesc.height = header->height;
    textureDesc.mipmapsCount = (std::max)(1u, header->mipMapCount);
    textureDesc.fmt = fmt;
    textureDesc.data.assign(fileData.begin() + dataOffset, fileData.end());

    return true;
}

void FillTextureSubresourceData(
    DXGI_FORMAT fmt,
    UINT32 width,
    UINT32 height,
    UINT32 mipLevels,
    const unsigned char* pSrcData,
    std::vector<D3D11_SUBRESOURCE_DATA>& outData)
{
    outData.clear();
    outData.resize(mipLevels);

    UINT32 curWidth = width;
    UINT32 curHeight = height;
    const unsigned char* pCur = pSrcData;

    for (UINT32 i = 0; i < mipLevels; ++i)
    {
        outData[i].pSysMem = pCur;

        UINT32 pitch = 0;
        UINT32 sliceSize = 0;

        if (IsBlockCompressed(fmt))
        {
            const UINT32 blockWidth = DivUp(curWidth, 4u);
            const UINT32 blockHeight = DivUp(curHeight, 4u);
            pitch = blockWidth * GetBytesPerBlock(fmt);
            sliceSize = pitch * blockHeight;
        }
        else
        {
            pitch = curWidth * GetBytesPerPixel(fmt);
            sliceSize = pitch * curHeight;
        }

        outData[i].SysMemPitch = pitch;
        outData[i].SysMemSlicePitch = sliceSize;

        pCur += sliceSize;
        curWidth = (std::max)(1u, curWidth / 2u);
        curHeight = (std::max)(1u, curHeight / 2u);
    }
}