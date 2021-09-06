#pragma once

#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include <random>
#include <chrono>
#include <array>

#include "pch.h"
#include "DeviceManager.h"
#include "CBuffer.h"
#include "BufferStructures.h"
#include "RaytracingShadersHelper.h"
#include "ModelClass.h"
#include "DepthStencilManager.h"
#include "PipelineStateManager.h"

using namespace DirectX;
//typedef std::array<D3D12_INPUT_ELEMENT_DESC, 6> BasicInputLayout;

// DEFINES
#define ROOT_SIGNATURE_PIXEL D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;// | //D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

class Renderer
{
	friend class Main;
	friend class GuiManager;

public:
	struct ScreenSize { UINT x, y; };
	ScreenSize GetWindowSize() const { return m_windowSize; };

	// Main events occuring in renderer - init/loop/destroy
	void OnInit(HWND hwnd);
	void OnUpdate();
	void OnRender();
	void OnDestroy();

	void LoadPipeline(HWND hwnd);
	void LoadAssets();

	void CreateRootSignatureRTCP(UINT rootParamCount, UINT samplerCount, CD3DX12_ROOT_PARAMETER rootParameters[], CD3DX12_STATIC_SAMPLER_DESC samplers[], D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags, ComPtr<ID3D12RootSignature>& rootSignature);

	void InitShaderCompiler(D3D12ShaderCompilerInfo& shaderCompiler) const;

	void CreateTexture2D(ComPtr<ID3D12Resource>& texture, UINT64 width, UINT height, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_TEXTURE_LAYOUT layout = D3D12_TEXTURE_LAYOUT_UNKNOWN, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	void CreateTextureFromFileRTCP(ComPtr<ID3D12Resource>& texture, ComPtr<ID3D12GraphicsCommandList4> commandList, const wchar_t* path, ComPtr<ID3D12Resource>& uploadHeap, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATES InitialResourceState = D3D12_RESOURCE_STATE_COPY_SOURCE);

	void AddCameraPosition(float x, float y, float z);
	void AddCameraPosition(XMFLOAT3 addPos);
	void AddCameraRotation(float x, float y, float z);
	void AddCameraRotation(XMFLOAT3 addRot);

	void CreateViewAndPerspective();

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob) const;
	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program) const;
	void Compile_Shader(_In_ LPCWSTR pFileName, _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines, _In_opt_ ID3DInclude* pInclude, _In_ LPCSTR pEntrypoint, _In_ LPCSTR pTarget, _In_ UINT Flags1, _In_ UINT Flags2, _Out_ ID3DBlob** ppCode) const;

private:
	std::array<D3D12_INPUT_ELEMENT_DESC, MAX_INPUT_ELEMENT_DESC> CreateBasicInputLayout();

	// Executing commands/synchronization functions
	void PopulateCommandList();
	void CloseCommandList();
	void WaitForPreviousFrame();
	void MoveToNextFrame();

	static void CreateUAV(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvUavCbvHeap, int resourceIdx, ID3D12Device* device, D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc);
	static void CreateSRV(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc);
	static void CreateSRV_Texture2D(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels = 1, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING });
	static void CreateSRV_Texture2DArray(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels = 1, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE2DARRAY, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING });
	static void CreateSRV_Texture3D(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels = 1, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURE3D, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING });
	static void CreateSRV_TextureCube(ComPtr<ID3D12Resource>& resource, ID3D12DescriptorHeap* srvHeap, int srvIndex, ID3D12Device* device, int mipLevels = 1, D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = { DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_SRV_DIMENSION_TEXTURECUBE, D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING });

private:
	static constexpr int m_frameCount = 2;
	static constexpr float Z_NEAR = 1.0f;
	static constexpr float Z_FAR = 100.0f;
	bool FREEZE_CAMERA = false;

	// Camera settings
	float m_cameraSpeed = 0.25f;
	XMFLOAT3 m_cameraPosition{ 0,0,0 };
	XMFLOAT3 m_cameraRotation{ 0,0,0 };
	XMFLOAT3 m_cameraPositionStoredInFrame{ 0,0,0 };

	ScreenSize m_windowSize{ 1024, 512 };

	// Frame data
	UINT64 m_currentCPUFrame = 0;
	UINT64 m_currentGPUFrame = 0;
	UINT64 m_currentFrameIdx = 0;

	// Viewport related variables
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	// Pipeline variables - device, commandQueue, swap chain
	int m_frameIndex = 0;
	ComPtr<ID3D12Device5> m_device = NULL;
	ComPtr<ID3D12CommandQueue> m_commandQueue = NULL;
	ComPtr<IDXGISwapChain3> m_swapChain = NULL;

	UINT m_rtvDescriptorSize = 0;

	// Command lists
	ComPtr<ID3D12GraphicsCommandList4> m_commandList = NULL;
	ComPtr<ID3D12GraphicsCommandList4> m_commandListSkybox = NULL;

	// Command allocators
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[m_frameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocatorsSkybox[m_frameCount];

	// Descriptor heaps
	ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;

	// Depth stencil view (DSV)
	ComPtr<ID3D12Resource> m_depthStencil;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	// Models
	std::shared_ptr<ModelClass> m_modelSphere = NULL;
	std::shared_ptr<ModelClass> m_modelCube = NULL;
	std::shared_ptr<ModelClass> m_modelFullscreen = NULL;
	
	// Textures
	ComPtr<ID3D12Resource> m_backBuffers[m_frameCount];
	ComPtr<ID3D12Resource> m_pebblesTexture;
	ComPtr<ID3D12Resource> m_skyboxTexture;
	ComPtr<ID3D12Resource> m_depthBuffer;
	ComPtr<ID3D12Resource> m_hiZBuffer;

	// Root signatures/PSO
	std::shared_ptr<PipelineStateManager> m_psoManager = NULL;
	ComPtr<ID3D12RootSignature> m_rootSignature = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineState = NULL;
	ComPtr<ID3D12RootSignature> m_rootSignatureSkybox = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineStateSkybox = NULL;
	ComPtr<ID3D12RootSignature> m_rootSignatureHiZ = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineStateHiZMipZero = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineStateHiZ = NULL;

	// Shader compiler
	D3D12ShaderCompilerInfo m_shaderCompiler{};

	// Constant buffers
	CBuffer<SceneConstantBuffer> m_sceneBuffer;
	CBuffer<CameraConstantBuffer> m_cameraBuffer;
	CBuffer<AoConstantBuffer> m_aoBuffer;
	CBuffer<GiConstantBuffer> m_giBuffer;
	CBuffer<ConstantBufferStruct> m_constantBuffer;
	CBuffer<ConstantBufferStruct> m_constantBufferSkybox;
	CBuffer<LightConstantBuffer> m_lightBuffer;
	CBuffer<PostprocessConstantBuffer> m_postprocessBuffer;
	CBuffer<HiZConstantBuffer> m_hizConstantBuffer;

	// Synchronization
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[m_frameCount];
	HANDLE m_fenceEvent;
};