#include "DeviceManager.h"

ComPtr<IDXGISwapChain3> DeviceManager::CreateSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<IDXGIFactory4> dxgiFactory, int width, int height, int bufferCount)
{
    assert(commandQueue);
    assert(dxgiFactory);
    assert(width > 0 && height > 0);
    assert(bufferCount > 0);

    ComPtr<IDXGISwapChain3> swapChain3;

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Windowed = TRUE;

    ComPtr<IDXGISwapChain> swapChain;
    ThrowIfFailed(dxgiFactory->CreateSwapChain(
        commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        &swapChainDesc,
        &swapChain
    ));

    ThrowIfFailed(swapChain.As(&swapChain3));

    return swapChain3;
}

ComPtr<ID3D12Device5> DeviceManager::CreateDevice(ComPtr<IDXGIFactory4> dxgiFactory, bool useWarp /* = false */)
{
    ComPtr<ID3D12Device5> device;

    if (useWarp)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
        ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(dxgiFactory.Get(), &hardwareAdapter);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
        ));
    }

#if defined(_DEBUG_YARE)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        //pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        D3D12_MESSAGE_SEVERITY severities[]{ D3D12_MESSAGE_SEVERITY_INFO };

        D3D12_MESSAGE_ID denyIDs[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER newFilter{};
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIDs);
        newFilter.DenyList.pIDList = denyIDs;

        ThrowIfFailed(pInfoQueue->PushStorageFilter(&newFilter));
    }
#endif
    return device;
}

ComPtr<ID3D12CommandQueue> DeviceManager::CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    assert(device);

    ComPtr<ID3D12CommandQueue> commandQueue;

    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));

    return commandQueue;
}

ComPtr<ID3D12DescriptorHeap> DeviceManager::CreateDescriptorHeap(ComPtr<ID3D12Device2> device, int numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags /* = D3D12_DESCRIPTOR_HEAP_FLAG_NONE */)
{
    assert(device);

    ComPtr<ID3D12DescriptorHeap> heap;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.Flags = flags;

    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

    return heap;
}

ComPtr<ID3D12CommandAllocator> DeviceManager::CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    assert(device);

    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList4> DeviceManager::CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    assert(device);
    assert(commandAllocator);

    ComPtr<ID3D12GraphicsCommandList4> commandList;
    ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    ThrowIfFailed(commandList->Close());

    return commandList;
}

ComPtr<ID3D12Resource> DeviceManager::CreateDepthStencilView(ComPtr<ID3D12Device2> device, ComPtr<ID3D12DescriptorHeap> dsvHeap, DXGI_FORMAT format, int width, int height, float clearDepthValue /* = 1.0f */, UINT8 clearStencilValue /* = 0 */)
{
    assert(device);
    assert(dsvHeap);
    assert(dsvHeap->GetCPUDescriptorHandleForHeapStart().ptr);

    ComPtr<ID3D12Resource> depthStencil;

    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = format;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
    depthStencilDesc.Texture2D.MipSlice = 0;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = format;
    depthOptimizedClearValue.DepthStencil.Depth = clearDepthValue;
    depthOptimizedClearValue.DepthStencil.Stencil = clearStencilValue;

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(format, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&depthStencil)
    ));

    device->CreateDepthStencilView(depthStencil.Get(), &depthStencilDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

    return depthStencil;
}

void DeviceManager::GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;
    for (UINT adapterIndex = 0; ; ++adapterIndex)
    {
        IDXGIAdapter1* pAdapter = nullptr;
        if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
        {
            // No more adapters to enumerate.
            break;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the
        // actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
        {
            *ppAdapter = pAdapter;
            return;
        }
        pAdapter->Release();
    }
}
