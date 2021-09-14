#pragma once
#ifndef _PIPELINE_STATE_MANAGER_H_
#define _PIPELINE_STATE_MANAGER_H_

#include "pch.h"
#include <array>

#define MAX_INPUT_ELEMENT_DESC 5

class PipelineStateManager
{
public:
	PipelineStateManager(ComPtr<ID3D12Device2> device) : m_device(device) {};

	ComPtr<ID3D12PipelineState> CreateGraphicsPipelineState(/*ComPtr<ID3D12PipelineState> pipelineState,*/ D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc);
	bool CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK, D3D12_COMPARISON_FUNC depthCompFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL);
	bool CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK, D3D12_COMPARISON_FUNC depthCompFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL);
	bool CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK, D3D12_COMPARISON_FUNC depthCompFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL);

	static D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPSO(std::array<D3D12_INPUT_ELEMENT_DESC, MAX_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK, D3D12_COMPARISON_FUNC depthCompFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL, std::vector<DXGI_FORMAT> formats = { DXGI_FORMAT_R8G8B8A8_UNORM });


private:
	ComPtr<ID3D12Device2> m_device;

};

#endif // !_PIPELINE_STATE_MANAGER_H_
