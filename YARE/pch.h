#pragma once
#ifndef _PCH_H_
#define _PCH_H_

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#ifndef _DEBUG_YARE
#define _DEBUG_YARE
#endif // !_DEBUG_YARE

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(CreateWindow)
#undef CreateWindow
#endif

#include <wrl.h>
using namespace Microsoft::WRL;

#pragma comment(lib,"d3d12.lib")
#include <d3d12.h>

#pragma comment(lib,"dxgi.lib")
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#pragma comment(lib,"dxguid.lib")
#include "d3dx12.h"

#include <algorithm>
#include <cassert>
#include <chrono>

#include "Helpers.h"

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
#pragma comment(lib,"runtimeobject.lib")
#endif

#endif // !_PCH_H_
