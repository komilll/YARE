#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <exception>
#include "CBuffer.h"

#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)
#define ALIGN(_alignment, _val) (((_val + _alignment - 1) / _alignment) * _alignment)

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

inline void ThrowIfFailed(HRESULT hr, const wchar_t* msg)
{
    if (FAILED(hr))
    {
        OutputDebugString(msg);
        throw std::exception();
    }
}

inline void ThrowIfFalse(bool value)
{
    ThrowIfFailed(value ? S_OK : E_FAIL);
}

inline void ThrowIfFalse(bool value, const wchar_t* msg)
{
    ThrowIfFailed(value ? S_OK : E_FAIL, msg);
}

template<class T>
inline void CreateUploadHeapRTCP(ID3D12Device5* device, CBuffer<T>& cbuffer)
{
    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(T)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbuffer.resource)
    ));

    if (cbuffer.elementCount > 0)
    {
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(cbuffer.resource->Map(0, &readRange, reinterpret_cast<void**>(&cbuffer.ptr)));
        memcpy(cbuffer.ptr, &cbuffer.values[0], sizeof(T) * cbuffer.elementCount);
        cbuffer.resource->Unmap(0, &readRange);
    }
    else
    {
        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(cbuffer.resource->Map(0, &readRange, reinterpret_cast<void**>(&cbuffer.ptr)));
        memcpy(cbuffer.ptr, &cbuffer.value, sizeof(T));
        cbuffer.resource->Unmap(0, &readRange);
    }
}