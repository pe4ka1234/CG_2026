#pragma once

#include "DxCommon.h"

#include <string>
#include <vector>

HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name);
std::string WCSToMBS(const std::wstring& w);
bool ReadAllBytes(const std::wstring& path, std::vector<unsigned char>& out);
HRESULT CompileShaderFromFileMemory(
    const std::wstring& path,
    const std::string& entryPoint,
    const std::string& platform,
    ID3DBlob** ppCode,
    const std::vector<std::string>* pDefines = nullptr);