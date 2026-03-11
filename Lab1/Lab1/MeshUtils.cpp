#include "framework.h"
#include "MeshUtils.h"

#include <cmath>

void BuildSphereMesh(
    std::vector<SkyVertex>& vertices,
    std::vector<UINT16>& indices,
    UINT32 slices,
    UINT32 stacks)
{
    vertices.clear();
    indices.clear();

    constexpr float PI = 3.14159265358979323846f;

    for (UINT32 stack = 0; stack <= stacks; ++stack)
    {
        const float v = static_cast<float>(stack) / static_cast<float>(stacks);
        const float phi = v * PI;

        const float y = cosf(phi);
        const float r = sinf(phi);

        for (UINT32 slice = 0; slice <= slices; ++slice)
        {
            const float u = static_cast<float>(slice) / static_cast<float>(slices);
            const float theta = u * 2.0f * PI;

            const float x = r * cosf(theta);
            const float z = r * sinf(theta);
            vertices.push_back({ x, y, z });
        }
    }

    const UINT32 rowSize = slices + 1;
    for (UINT32 stack = 0; stack < stacks; ++stack)
    {
        for (UINT32 slice = 0; slice < slices; ++slice)
        {
            const UINT16 i0 = static_cast<UINT16>(stack * rowSize + slice);
            const UINT16 i1 = static_cast<UINT16>(i0 + 1);
            const UINT16 i2 = static_cast<UINT16>((stack + 1) * rowSize + slice);
            const UINT16 i3 = static_cast<UINT16>(i2 + 1);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }
}