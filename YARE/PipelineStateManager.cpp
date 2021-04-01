#include "PipelineStateManager.h"

ComPtr<ID3D12PipelineState> PipelineStateManager::CreateGraphicsPipelineState(/*ComPtr<ID3D12PipelineState> pipelineState, */D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc)
{
    ComPtr<ID3D12PipelineState> pipelineState = NULL;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));

    return pipelineState;
}

bool PipelineStateManager::CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode, D3D12_COMPARISON_FUNC depthCompFunc)
{
    return false;
}

bool PipelineStateManager::CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode, D3D12_COMPARISON_FUNC depthCompFunc)
{
    return false;
}

bool PipelineStateManager::CreateGraphicsPipelineState(ComPtr<ID3D12PipelineState> pipelineState, std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode, D3D12_COMPARISON_FUNC depthCompFunc)
{
    return false;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineStateManager::CreateDefaultPSO(std::array<D3D12_INPUT_ELEMENT_DESC, MAX_INPUT_ELEMENT_DESC> inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature, D3D12_CULL_MODE cullMode /* = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK */, D3D12_COMPARISON_FUNC depthCompFunc /* = D3D12_COMPARISON_FUNC_LESS_EQUAL */)
{
    // Set correct size of input elements array
    int inputElementSize = inputElementDescs.size();
    for (const auto& el : inputElementDescs) {
        if (el.SemanticName == NULL)
        {
            --inputElementSize;
        }
    }

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { &inputElementDescs[0], static_cast<UINT>(inputElementSize) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.CullMode = cullMode;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DepthStencilState.DepthFunc = depthCompFunc;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    return psoDesc;
}