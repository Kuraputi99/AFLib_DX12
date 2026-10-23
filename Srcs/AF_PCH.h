#ifndef AF_PCH_H
#define AF_PCH_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif // !NOMINMAX

#include <Windows.h>
#include <string>
#include <string_view>
#include <format>
#include <cstdio>
#include <memory>

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#endif // !AF_PCH_H
