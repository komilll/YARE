#pragma once

#include "pch.h"

class DeviceManager
{
public:
	static ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIFactory4> dxgiFactory, bool useWarp = false);
	static ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	static ComPtr<IDXGISwapChain3> CreateSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<IDXGIFactory4> dxgiFactory, int width, int height, int bufferCount);

private:
	ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, int numDescriptors) const;
	ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) const;
	ComPtr<ID3D12GraphicsCommandList4> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type) const;

	static void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter);
};