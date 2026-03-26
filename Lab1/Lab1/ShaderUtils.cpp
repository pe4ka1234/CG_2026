#include "framework.h"
#include "ShaderUtils.h"

#include <assert.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstdlib>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    class D3DInclude final : public ID3DInclude
    {
    public:
        HRESULT STDMETHODCALLTYPE Open(
            D3D_INCLUDE_TYPE IncludeType,
            LPCSTR pFileName,
            LPCVOID pParentData,
            LPCVOID* ppData,
            UINT* pBytes) override
        {
            UNREFERENCED_PARAMETER(IncludeType);
            UNREFERENCED_PARAMETER(pParentData);

            if (!pFileName || !ppData || !pBytes)
                return E_INVALIDARG;

            FILE* pFile = nullptr;
            if (fopen_s(&pFile, pFileName, "rb") != 0 || !pFile)
                return E_FAIL;

            fseek(pFile, 0, SEEK_END);
            const long size = ftell(pFile);
            fseek(pFile, 0, SEEK_SET);

            if (size <= 0)
            {
                fclose(pFile);
                return E_FAIL;
            }

            void* pData = std::malloc(static_cast<size_t>(size));
            if (!pData)
            {
                fclose(pFile);
                return E_OUTOFMEMORY;
            }

            const size_t readBytes = fread(pData, 1, static_cast<size_t>(size), pFile);
            fclose(pFile);

            if (readBytes != static_cast<size_t>(size))
            {
                std::free(pData);
                return E_FAIL;
            }

            *ppData = pData;
            *pBytes = static_cast<UINT>(size);
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Close(LPCVOID pData) override
        {
            std::free(const_cast<void*>(pData));
            return S_OK;
        }
    };
}

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
    ID3DBlob** ppCode,
    const std::vector<std::string>* pDefines)
{
    if (!ppCode)
        return E_INVALIDARG;

    *ppCode = nullptr;

    std::vector<unsigned char> data;
    if (!ReadAllBytes(path, data))
        return E_FAIL;

    std::vector<D3D_SHADER_MACRO> shaderDefines;
    if (pDefines && !pDefines->empty())
    {
        shaderDefines.resize(pDefines->size() + 1);
        for (size_t i = 0; i < pDefines->size(); ++i)
        {
            shaderDefines[i].Name = (*pDefines)[i].c_str();
            shaderDefines[i].Definition = "";
        }

        shaderDefines.back().Name = nullptr;
        shaderDefines.back().Definition = nullptr;
    }

    UINT flags1 = 0;
#if defined(_DEBUG)
    flags1 |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    D3DInclude includeHandler;
    ID3DBlob* pErrMsg = nullptr;
    const HRESULT result = D3DCompile(
        data.data(),
        data.size(),
        WCSToMBS(path).c_str(),
        shaderDefines.empty() ? nullptr : shaderDefines.data(),
        &includeHandler,
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