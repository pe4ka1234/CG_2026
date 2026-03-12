#include "framework.h"
#include "ShaderUtils.h"

#include <assert.h>
#include <d3dcompiler.h>
#include <cstdio>

#pragma comment(lib, "d3dcompiler.lib")

HRESULT SetResourceName(ID3D11DeviceChild* pResource, const std::string& name)
{
    if (!pResource)
        return E_INVALIDARG;

    return pResource->SetPrivateData(
        WKPDID_D3DDebugObjectName,
        static_cast<UINT>(name.length()),
        name.c_str());
}

std::string WCSToMBS(const std::wstring& w)
{
    if (w.empty())
        return std::string();

    const int sizeNeeded = WideCharToMultiByte(
        CP_UTF8,
        0,
        w.c_str(),
        static_cast<int>(w.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (sizeNeeded <= 0)
        return std::string();

    std::string result(static_cast<size_t>(sizeNeeded), '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        w.c_str(),
        static_cast<int>(w.size()),
        &result[0],
        sizeNeeded,
        nullptr,
        nullptr);

    return result;
}

bool ReadAllBytes(const std::wstring& path, std::vector<unsigned char>& out)
{
    out.clear();

    FILE* pFile = nullptr;
    if (_wfopen_s(&pFile, path.c_str(), L"rb") != 0 || !pFile)
        return false;

    fseek(pFile, 0, SEEK_END);
    const long size = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    if (size <= 0)
    {
        fclose(pFile);
        return false;
    }

    out.resize(static_cast<size_t>(size));
    const size_t readBytes = fread(out.data(), 1, out.size(), pFile);
    fclose(pFile);

    return readBytes == out.size();
}

HRESULT CompileShaderFromFileMemory(
    const std::wstring& path,
    const std::string& entryPoint,
    const std::string& platform,
    ID3DBlob** ppCode)
{
    if (!ppCode)
        return E_INVALIDARG;

    *ppCode = nullptr;

    std::vector<unsigned char> data;
    if (!ReadAllBytes(path, data))
        return E_FAIL;

    UINT flags1 = 0;
#if defined(_DEBUG)
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrMsg = nullptr;
    const HRESULT result = D3DCompile(
        data.data(),
        data.size(),
        WCSToMBS(path).c_str(),
        nullptr,
        nullptr,
        entryPoint.c_str(),
        platform.c_str(),
        flags1,
        0,
        ppCode,
        &pErrMsg);

    if (FAILED(result) && pErrMsg)
        OutputDebugStringA(static_cast<const char*>(pErrMsg->GetBufferPointer()));

    assert(SUCCEEDED(result));
    SAFE_RELEASE(pErrMsg);

    return result;
}