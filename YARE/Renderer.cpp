#include "Renderer.h"
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <DirectXHelpers.h>
#include <DirectXTex.h>
#include <SimpleMath.h>

void Renderer::OnInit(HWND hwnd)
{
    // Prepare viewport and camera data
    m_viewport.TopLeftY = 0.0f;
    m_viewport.TopLeftX = 0.0f;
    m_viewport.Width = static_cast<float>(m_windowSize.x);
    m_viewport.Height = static_cast<float>(m_windowSize.y);

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(m_windowSize.x);
    m_scissorRect.bottom = static_cast<LONG>(m_windowSize.y);

    m_cameraPosition = XMFLOAT3{ 0, 0, -60.0f };

    // Create initial view and perspective matrix
    CreateViewAndPerspective();

    // Preparing devices, resources, views to enable rendering
    LoadPipeline(hwnd);
    LoadAssets();

    //
    CreateUploadHeapRTCP(m_device.Get(), m_sceneBuffer);
}

// Perform changes in the scene before calling rendering functions
void Renderer::OnUpdate()
{
    // Update vertexShader.hlsl
    {
        // Update data for single rasterized object
        m_constantBuffer.value.world = XMMatrixIdentity();
        m_constantBuffer.Update();

        // Update data for skybox rendering
        m_constantBufferSkybox.value = m_constantBuffer.value;
        XMFLOAT3 pos = XMFLOAT3{ m_cameraPosition.x - 0.5f, m_cameraPosition.y - 0.5f, m_cameraPosition.z - 0.5f };
        m_constantBufferSkybox.value.world = XMMatrixIdentity();
        m_constantBufferSkybox.value.world = XMMatrixMultiply(m_constantBufferSkybox.value.world, XMMatrixScaling(1.0f, 1.0f, 1.0f));
        m_constantBufferSkybox.value.world = XMMatrixMultiply(m_constantBufferSkybox.value.world, XMMatrixTranslation(pos.x, pos.y, pos.z));
        m_constantBufferSkybox.value.world = XMMatrixTranspose(m_constantBufferSkybox.value.world);
        m_constantBufferSkybox.Update();
    }

    // Update raygeneration cbuffer - scene CB
    {
        XMMATRIX viewProj = XMMatrixMultiply(XMMatrixTranspose(m_constantBuffer.value.view), XMMatrixTranspose(m_constantBuffer.value.projection));
        m_sceneBuffer.value.projectionToWorld = XMMatrixTranspose(XMMatrixInverse(nullptr, viewProj));
        m_sceneBuffer.value.frameCount++;
        m_sceneBuffer.value.frameCount %= INT_MAX;
        m_sceneBuffer.Update();
    }
}

void Renderer::OnRender()
{
    // Prepare commands to execute
    //PopulateCommandList(); // Moved to main.cpp
    
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get(), m_commandListSkybox.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    m_currentCPUFrame++;

    ThrowIfFailed(m_swapChain->Present(1, 0));
    MoveToNextFrame();

    m_currentFrameIdx = m_currentCPUFrame % m_frameCount;
}

void Renderer::OnDestroy()
{
    // Wait for the GPU to be done with all resources.
    WaitForPreviousFrame();

    CloseHandle(m_fenceEvent);
}

void Renderer::LoadPipeline(HWND hwnd)
{
    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG_YARE)
    // Enable the D3D12 debug layer.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    // Create factory which will be used throughout function
    ComPtr<IDXGIFactory4> dxgiFactory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

    // Create device, command queue and swap chain
    m_device = DeviceManager::CreateDevice(dxgiFactory);
    m_commandQueue = DeviceManager::CreateCommandQueue(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_swapChain = DeviceManager::CreateSwapChain(hwnd, m_commandQueue, dxgiFactory, static_cast<int>(m_windowSize.x), static_cast<int>(m_windowSize.y), m_frameCount);

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(dxgiFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    // Store current index of back buffer
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Store DX12 size of HEAP_TYPE_RTV
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        m_rtvDescriptorHeap = DeviceManager::CreateDescriptorHeap(m_device, m_frameCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

        // Describe and create a shader resource view (SRV) heap for the texture.
        m_srvHeap = DeviceManager::CreateDescriptorHeap(m_device, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);

        // Describe and create a depth stencil view (DSV) descriptor heap.
        m_dsvHeap = DeviceManager::CreateDescriptorHeap(m_device, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    }

    // Create the depth stencil view.
    {
        //D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
        //depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
        //depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        //depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

        //D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        //depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        //depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        //depthOptimizedClearValue.DepthStencil.Stencil = 0;

        //ThrowIfFailed(m_device->CreateCommittedResource(
        //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        //    D3D12_HEAP_FLAG_NONE,
        //    &CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, static_cast<UINT>(m_windowSize.x), static_cast<UINT>(m_windowSize.y), 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        //    D3D12_RESOURCE_STATE_DEPTH_WRITE,
        //    &depthOptimizedClearValue,
        //    IID_PPV_ARGS(&m_depthStencil)
        //));

        //m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        m_depthStencil = DeviceManager::CreateDepthStencilView(m_device, m_dsvHeap, DXGI_FORMAT_D32_FLOAT, static_cast<int>(m_windowSize.x), static_cast<int>(m_windowSize.y));
    }

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < m_frameCount; n++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_backBuffers[n])));
            m_device->CreateRenderTargetView(m_backBuffers[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[n])));
            ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocatorsSkybox[n])));
        }
    }
}

void Renderer::LoadAssets()
{
    // Create an empty root signature.
    {
        CD3DX12_ROOT_PARAMETER rootParameters[2] = {};
        CD3DX12_DESCRIPTOR_RANGE range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
        rootParameters[0].InitAsDescriptorTable(1, &range);
        rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = ROOT_SIGNATURE_PIXEL;

        CD3DX12_STATIC_SAMPLER_DESC samplers[1] = {};
        samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

        CreateRootSignatureRTCP(_countof(rootParameters), _countof(samplers), rootParameters, samplers, rootSignatureFlags, m_rootSignature);
    }

    // Create an empty root signature. - SKYBOX
    {
        CD3DX12_ROOT_PARAMETER rootParameters[3] = {};
        CD3DX12_DESCRIPTOR_RANGE range{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 };
        rootParameters[0].InitAsDescriptorTable(1, &range);
        rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        rootParameters[2].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = ROOT_SIGNATURE_PIXEL;

        CD3DX12_STATIC_SAMPLER_DESC samplers[1] = {};
        samplers[0].Init(0, D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);

        CreateRootSignatureRTCP(_countof(rootParameters), _countof(samplers), rootParameters, samplers, rootSignatureFlags, m_rootSignatureSkybox);
    }

#if defined(_DEBUG_YARE)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        // Prepare shaders
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        Compile_Shader(L"Shaders/vertexShader.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_1", 0, 0, &vertexShader);
        Compile_Shader(L"Shaders/pixelShader.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_1", 0, 0, &pixelShader);

        // Preprare layout, DSV and create PSO
        auto inputElementDescs = CreateBasicInputLayout();
        CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc = CreateDefaultDepthStencilDesc();
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateDefaultPSO(inputElementDescs, vertexShader, pixelShader, depthStencilDesc, m_rootSignature);
        psoDesc.RasterizerState.FrontCounterClockwise = true;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
    }

    // Create the pipeline state, which includes compiling and loading shaders. - SKYBOX
    {
        // Prepare shaders
        ComPtr<ID3DBlob> vertexShader;
        ComPtr<ID3DBlob> pixelShader;
        Compile_Shader(L"Shaders/VS_Skybox.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "vs_5_1", 0, 0, &vertexShader);
        Compile_Shader(L"Shaders/PS_Skybox.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", "ps_5_1", 0, 0, &pixelShader);

        // Preprare layout, DSV and create PSO
        auto inputElementDescs = CreateBasicInputLayout();
        CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc = CreateDefaultDepthStencilDesc();

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = CreateDefaultPSO(inputElementDescs, vertexShader, pixelShader, depthStencilDesc, m_rootSignatureSkybox);
        psoDesc.RasterizerState.FrontCounterClockwise = true;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

        ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateSkybox)));
    }

    // Create the command list.
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList)));
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorsSkybox[m_frameIndex].Get(), m_pipelineStateSkybox.Get(), IID_PPV_ARGS(&m_commandListSkybox)));

    ComPtr<ID3D12Resource> modelHeap;
    // Create the vertex buffer.
    {
        m_modelCube = std::shared_ptr<ModelClass>(new ModelClass("cube.obj", m_device, m_commandList));
        m_modelSphere = std::shared_ptr<ModelClass>(new ModelClass("sphere.obj", m_device, m_commandList));
        //m_modelBuddha = std::shared_ptr<ModelClass>(new ModelClass("happy-buddha.fbx", m_device, m_commandList));
        //m_modelPinkRoom = std::shared_ptr<ModelClass>(new ModelClass("SunTemple.fbx", m_device, m_commandList, modelHeap));
        //m_modelFullscreen = std::shared_ptr<ModelClass>(new ModelClass());
        //m_modelFullscreen->SetFullScreenRectangleModel(m_device, m_commandList);
    }

    // Prepare shader compilator
    InitShaderCompiler(m_shaderCompiler);

    // Create the constant buffer
    {
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBufferStruct)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer.resource)
        ));

        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_constantBuffer.resource->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBuffer.ptr)));
        memcpy(m_constantBuffer.ptr, &m_constantBuffer.value, sizeof(ConstantBufferStruct));
        m_constantBuffer.resource->Unmap(0, &readRange);
    }

    // Create the constant buffer - SKYBOX
    {
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBufferStruct)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBufferSkybox.resource)
        ));

        CD3DX12_RANGE readRange(0, 0);
        ThrowIfFailed(m_constantBufferSkybox.resource->Map(0, &readRange, reinterpret_cast<void**>(&m_constantBufferSkybox.ptr)));
        memcpy(m_constantBufferSkybox.ptr, &m_constantBufferSkybox.value, sizeof(ConstantBufferStruct));
        m_constantBufferSkybox.resource->Unmap(0, &readRange);
    }

#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
    if (FAILED(initialize)) {
        throw std::exception();
    }
    // error
#else
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        throw std::exception();
    }
    // error
#endif

    ComPtr<ID3D12Resource> uploadHeap;
    // Create texture for rasterized object
    {
        //CreateTextureFromFileRTCP(m_dfgTexture, m_commandList, L"DFG.dds", uploadHeap);
        CreateTextureFromFileRTCP(m_pebblesTexture, m_commandList, L"Pebles.png", uploadHeap, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        CreateSRV_Texture2D(m_pebblesTexture, m_srvHeap.Get(), 0, m_device.Get());

        //CreateSRV_Texture2DArray(m_modelPinkRoom->GetTextureResourcesAlbedo(), m_srvHeap.Get(), 0, m_device.Get());
        //CreateSRV_Texture2D(m_rtLambertTexture, m_srvHeap.Get(), 0, m_device.Get());
        //CreateSRV_Texture2D(m_rtGGXTexture, m_srvHeap.Get(), 0, m_device.Get());
        //CreateUploadHeapRTCP(m_device.Get(), m_postprocessBuffer);
    }

    ComPtr<ID3D12Resource> skyboxUploadHeap;
    // Create skybox texture
    {
        CreateTextureFromFileRTCP(m_skyboxTexture, m_commandListSkybox, L"Skyboxes/cubemap.dds", skyboxUploadHeap);
        CreateSRV_TextureCube(m_skyboxTexture, m_srvHeap.Get(), 1, m_device.Get());
    }

    // Close the command list and execute it to begin the initial GPU setup.
    ThrowIfFailed(m_commandList->Close());
    ThrowIfFailed(m_commandListSkybox->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get(), m_commandListSkybox.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ThrowIfFailed(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceValues[m_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        WaitForPreviousFrame();
    }
}

BasicInputLayout Renderer::CreateBasicInputLayout()
{
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32_UINT, 0, 56, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    BasicInputLayout arr;
    std::move(std::begin(inputElementDescs), std::end(inputElementDescs), arr.begin());

    return arr;
}

CD3DX12_DEPTH_STENCIL_DESC1 Renderer::CreateDefaultDepthStencilDesc()
{
    CD3DX12_DEPTH_STENCIL_DESC1 depthStencilDesc{};
    depthStencilDesc.DepthBoundsTestEnable = true;

    // Set up the description of the stencil state.
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

    depthStencilDesc.StencilEnable = true;
    depthStencilDesc.StencilReadMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.StencilWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

    // Stencil operations if pixel is front-facing.
    depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
    depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    // Stencil operations if pixel is back-facing.
    depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
    depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

    return depthStencilDesc;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC Renderer::CreateDefaultPSO(BasicInputLayout inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature)
{
    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { &inputElementDescs[0], static_cast<UINT>(inputElementDescs.size()) };
    psoDesc.pRootSignature = rootSignature.Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = depthStencilDesc;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

    return psoDesc;
}

void Renderer::CreateRootSignatureRTCP(UINT rootParamCount, UINT samplerCount, CD3DX12_ROOT_PARAMETER rootParameters[], CD3DX12_STATIC_SAMPLER_DESC samplers[], D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags, ComPtr<ID3D12RootSignature>& rootSignature)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(rootParamCount, rootParameters, samplerCount, samplers, rootSignatureFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Renderer::InitShaderCompiler(D3D12ShaderCompilerInfo& shaderCompiler) const
{
    ThrowIfFailed(shaderCompiler.DxcDllHelper.Initialize());
    ThrowIfFailed(shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcCompiler, &shaderCompiler.compiler));
    ThrowIfFailed(shaderCompiler.DxcDllHelper.CreateInstance(CLSID_DxcLibrary, &shaderCompiler.library));
}

void Renderer::CreateTexture2D(ComPtr<ID3D12Resource>& texture, UINT64 width, UINT height, DXGI_FORMAT format, D3D12_TEXTURE_LAYOUT layout, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES InitialResourceState)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.DepthOrArraySize = 1;
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Format = format;
    desc.Flags = flags;
    desc.Width = width;
    desc.Height = height;
    desc.Layout = layout;
    desc.MipLevels = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &desc,
        InitialResourceState,
        nullptr,
        IID_PPV_ARGS(&texture)
    ));
}

void Renderer::CreateTextureFromFileRTCP(ComPtr<ID3D12Resource>& texture, ComPtr<ID3D12GraphicsCommandList4> commandList, const wchar_t* path, ComPtr<ID3D12Resource>& uploadHeap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES InitialResourceState)
{
    std::unique_ptr<uint8_t[]> decodedData;
    std::vector<D3D12_SUBRESOURCE_DATA> textureData;
    D3D12_SUBRESOURCE_DATA textureDataSingle;

    if (SUCCEEDED(LoadDDSTextureFromFileEx(m_device.Get(), path, 0, flags, DDS_LOADER_DEFAULT, texture.ReleaseAndGetAddressOf(), decodedData, textureData)))
    {
        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, static_cast<UINT>(textureData.size()));

        // uploadHeap must outlive this function - until command list is closed
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadHeap)
        ));

        UpdateSubresources(commandList.Get(), texture.Get(), uploadHeap.Get(), 0, 0, static_cast<UINT>(textureData.size()), &textureData[0]);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, InitialResourceState));
    }
    else
    {
        ThrowIfFailed(LoadWICTextureFromFileEx(m_device.Get(), path, 0, flags, WIC_LOADER_FORCE_RGBA32, texture.ReleaseAndGetAddressOf(), decodedData, textureDataSingle));

        const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
        auto desc = texture->GetDesc();

        // uploadHeap must outlive this function - until command list is closed
        ThrowIfFailed(m_device->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadHeap)
        ));

        //CreateTexture2D(texture, desc.Width, desc.Height, desc.Format, desc.Layout, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
        UpdateSubresources(commandList.Get(), texture.Get(), uploadHeap.Get(), 0, 0, 1, &textureDataSingle);
        commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, InitialResourceState));
    }
}

void Renderer::AddCameraPosition(float x, float y, float z)
{
    if (x != 0 || y != 0 || z != 0)
    {
        m_cameraPositionStoredInFrame.x = x;
        m_cameraPositionStoredInFrame.y = y;
        m_cameraPositionStoredInFrame.z = z;
        CreateViewAndPerspective();
    }
}

void Renderer::AddCameraPosition(XMFLOAT3 addPos)
{
    AddCameraPosition(addPos.x, addPos.y, addPos.z);
}

void Renderer::AddCameraRotation(float x, float y, float z)
{
    if (x != 0 || y != 0 || z != 0)
    {
        m_cameraRotation.x += x;
        m_cameraRotation.y += y;

        if (m_cameraRotation.y > 360.0f)
            m_cameraRotation.y -= 360.0f;
        else if (m_cameraRotation.y < -360.0f)
            m_cameraRotation.y += 360.0f;
        // Avoid gimbal lock problem, by avoiding reaching 90 deg
        m_cameraRotation.x = m_cameraRotation.x >= 90.0f ? 89.9f : (m_cameraRotation.x <= -90.0f ? -89.9f : m_cameraRotation.x);

        CreateViewAndPerspective();
    }
}

void Renderer::AddCameraRotation(XMFLOAT3 addRot)
{
    AddCameraPosition(addRot.x, addRot.y, addRot.z);
}

void Renderer::CreateViewAndPerspective()
{
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.f);
    constexpr float conv{ 0.0174532925f };

    // Create the rotation matrix from the yaw, pitch, and roll values.
    const XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_cameraRotation.x * conv, m_cameraRotation.y * conv, m_cameraRotation.z * conv);

    //Move camera along direction we look at
    if (m_cameraPositionStoredInFrame.x != 0 || m_cameraPositionStoredInFrame.y != 0 || m_cameraPositionStoredInFrame.z != 0)
    {
        const XMMATRIX YrotationMatrix = XMMatrixRotationY(m_cameraRotation.y * conv);
        const XMVECTOR camRight = XMVector3TransformCoord(XMVECTOR{ 1,0,0,0 }, YrotationMatrix);
        const XMVECTOR camForward = XMVector3TransformCoord(XMVECTOR{ 0, 0, 1, 0 }, rotationMatrix);

        const XMVECTOR addPos = camRight * m_cameraPositionStoredInFrame.x + camForward * m_cameraPositionStoredInFrame.z;
        m_cameraPosition.x += addPos.m128_f32[0];
        m_cameraPosition.y += (addPos.m128_f32[1] + m_cameraPositionStoredInFrame.y);
        m_cameraPosition.z += addPos.m128_f32[2];

        m_cameraPositionStoredInFrame = XMFLOAT3{ 0,0,0 };
        if (m_cameraPosition.x == 0.0f && m_cameraPosition.y == 0.0f && m_cameraPosition.z == 0.0f)
        {
            m_cameraPosition.x = FLT_MIN;
        }
    }
    const DirectX::XMVECTOR eye = DirectX::XMVectorSet(m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 0.0f);

    //Setup target (look at object position)
    XMVECTOR target = XMVector3TransformCoord(DirectX::XMVECTOR{ 0, 0, 1, 0 }, rotationMatrix);
    target = XMVector3Normalize(target);
    target = { m_cameraPosition.x + target.m128_f32[0], m_cameraPosition.y + target.m128_f32[1], m_cameraPosition.z + target.m128_f32[2], 0.0f };

    //Update camera position for shader buffer
    if (!FREEZE_CAMERA)
    {
        m_sceneBuffer.value.cameraPosition = XMFLOAT4{ m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z, 1.0f };
    }

    //Create view matrix
    m_constantBuffer.value.view = DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(eye, target, up));

    //Create perspective matrix
    constexpr float FOV = 3.14f / 4.0f;
    float aspectRatio = m_windowSize.x / m_windowSize.y;
    m_constantBuffer.value.projection = DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(FOV, aspectRatio, Z_NEAR, Z_FAR));
}

void Renderer::PopulateCommandList()
{
    //m_constantBufferSkybox.Update();
    //m_sceneBuffer.Update();

    //...


    ThrowIfFailed(m_commandAllocators[m_frameIndex]->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get()));

    m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* ppHeaps[] = { m_srvHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle0(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle1(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_commandList->SetGraphicsRootDescriptorTable(0, srvHandle0);

    m_commandList->SetGraphicsRootConstantBufferView(1, m_constantBuffer.resource->GetGPUVirtualAddress());
    //m_commandList->SetGraphicsRootConstantBufferView(2, m_constantBuffers[1]->GetGPUVirtualAddress());

    m_commandList->RSSetViewports(1, &m_viewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffers[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->ClearDepthStencilView(m_dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_modelSphere->GetVertexBufferView());
    m_commandList->DrawInstanced(m_modelSphere->GetIndicesCount(), 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffers[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

    //////////////////////
    // DRAW SKYBOX
    ThrowIfFailed(m_commandAllocatorsSkybox[m_frameIndex]->Reset());
    ThrowIfFailed(m_commandListSkybox->Reset(m_commandAllocatorsSkybox[m_frameIndex].Get(), m_pipelineStateSkybox.Get()));

    m_commandListSkybox->SetGraphicsRootSignature(m_rootSignatureSkybox.Get());
    m_commandListSkybox->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    m_commandListSkybox->SetGraphicsRootDescriptorTable(0, srvHandle1);

    m_commandListSkybox->SetGraphicsRootConstantBufferView(1, m_constantBufferSkybox.resource->GetGPUVirtualAddress());
    m_commandListSkybox->SetGraphicsRootConstantBufferView(2, m_sceneBuffer.resource->GetGPUVirtualAddress());

    m_commandListSkybox->RSSetViewports(1, &m_viewport);
    m_commandListSkybox->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    m_commandListSkybox->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffers[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

    m_commandListSkybox->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    // Record commands.
    m_commandListSkybox->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandListSkybox->IASetVertexBuffers(0, 1, &m_modelCube->GetVertexBufferView());
    m_commandListSkybox->DrawInstanced(m_modelCube->GetIndicesCount(), 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    m_commandListSkybox->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_backBuffers[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    //////////////////////
}

void Renderer::CloseCommandList()
{
    ThrowIfFailed(m_commandList->Close());
    ThrowIfFailed(m_commandListSkybox->Close());
}

void Renderer::WaitForPreviousFrame()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_frameIndex]++;
}

void Renderer::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
        m_currentGPUFrame++;
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}

void Renderer::CreateSRV(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(srvHeap->GetCPUDescriptorHandleForHeapStart(), srvIndex, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
}

void Renderer::CreateSRV_Texture2D(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
    srvDesc.Texture2D.MipLevels = mipLevels;
    CreateSRV(resource, srvHeap, srvIndex, device, srvDesc);
}

void Renderer::CreateSRV_Texture2DArray(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
    srvDesc.Texture2DArray.MipLevels = mipLevels;
    srvDesc.Texture2DArray.ArraySize = 36;
    CreateSRV(resource, srvHeap, srvIndex, device, srvDesc);
}

void Renderer::CreateSRV_Texture3D(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
    srvDesc.Texture3D.MipLevels = mipLevels;
    CreateSRV(resource, srvHeap, srvIndex, device, srvDesc);
}

void Renderer::CreateSRV_TextureCube(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc)
{
    srvDesc.TextureCube.MipLevels = mipLevels;
    CreateSRV(resource, srvHeap, srvIndex, device, srvDesc);
}

void Renderer::Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob) const
{
    HRESULT hr;
    UINT32 code(0);
    IDxcBlobEncoding* pShaderText(nullptr);

    // Load and encode the shader file
    ThrowIfFailed(compilerInfo.library->CreateBlobFromFile(info.filename, &code, &pShaderText));

    // Create the compiler include handler
    ComPtr<IDxcIncludeHandler> dxcIncludeHandler;
    ThrowIfFailed(compilerInfo.library->CreateIncludeHandler(&dxcIncludeHandler));

    // Compile the shader
    IDxcOperationResult* result;
    ThrowIfFailed(compilerInfo.compiler->Compile(
        pShaderText,
        info.filename,
        info.entryPoint,
        info.targetProfile,
        info.arguments,
        info.argCount,
        info.defines,
        info.defineCount,
        dxcIncludeHandler.Get(),
        &result
    ));

    // Verify the result
    result->GetStatus(&hr);
    if (FAILED(hr))
    {
        IDxcBlobEncoding* error;
        ThrowIfFailed(result->GetErrorBuffer(&error));

        // Convert error blob to a string
        std::vector<char> infoLog(error->GetBufferSize() + 1);
        memcpy(infoLog.data(), error->GetBufferPointer(), error->GetBufferSize());
        infoLog[error->GetBufferSize()] = 0;

        std::string errorMsg = "Shader Compiler Error:\n";
        errorMsg.append(infoLog.data());

        MessageBoxA(nullptr, errorMsg.c_str(), "Error!", MB_OK);
        return;
    }

    ThrowIfFailed(result->GetResult(blob));
}

void Renderer::Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program) const
{
    Compile_Shader(compilerInfo, program.info, &program.blob);
    program.SetBytecode();
}

void Renderer::Compile_Shader(LPCWSTR pFileName, const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode) const
{
    ComPtr<ID3DBlob> errorBlob;
    HRESULT result = D3DCompileFromFile(pFileName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, &errorBlob);
    if (FAILED(result))
    {
        if (errorBlob) {
            MessageBoxA(nullptr, (char*)errorBlob->GetBufferPointer(), "HLSL error", MB_OK);
            errorBlob->Release();
        }
        ThrowIfFailed(result);
    }
}
