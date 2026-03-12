#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcommon.h>
#include <DirectXMath.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) do { if ((p) != nullptr) { (p)->Release(); (p) = nullptr; } } while (0)
#endif