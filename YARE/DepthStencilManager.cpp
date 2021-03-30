#include "DepthStencilManager.h"

CD3DX12_DEPTH_STENCIL_DESC1 DepthStencilManager::CreateDefaultDepthStencilDesc()
{
    CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc(D3D12_DEFAULT);
    depthStencilDesc.DepthBoundsTestEnable = FALSE;

    // Set up the description of the stencil state.
    depthStencilDesc.DepthEnable = TRUE;
    //depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    //depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    //depthStencilDesc.StencilEnable = true;
    //depthStencilDesc.StencilReadMask = D3D12_DEPTH_WRITE_MASK_ALL;
    //depthStencilDesc.StencilWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    //// Stencil operations if pixel is front-facing.
    //depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    //depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
    //depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    //depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    //// Stencil operations if pixel is back-facing.
    //depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    //depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
    //depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    //depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    return depthStencilDesc;
}
