#pragma once

#include "DxCommon.h"
#include "RenderTypes.h"

#include <vector>

void BuildSphereMesh(
    std::vector<SkyVertex>& vertices,
    std::vector<UINT16>& indices,
    UINT32 slices = 32,
    UINT32 stacks = 16);