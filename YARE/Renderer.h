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

using namespace DirectX;
typedef std::array<D3D12_INPUT_ELEMENT_DESC, 6> BasicInputLayout;

// DEFINES
#define ROOT_SIGNATURE_PIXEL D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;// | //D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

class Renderer
{
public:
	XMFLOAT2 GetWindowSize() const { return m_windowSize; };

	// Main events occuring in renderer - init/loop/destroy
	void OnInit(HWND hwnd);
	void OnUpdate();
	void OnRender();
	void OnDestroy();

	void LoadPipeline(HWND hwnd);
	void LoadAssets();


	CD3DX12_DEPTH_STENCIL_DESC1 CreateDefaultDepthStencilDesc();

	D3D12_GRAPHICS_PIPELINE_STATE_DESC CreateDefaultPSO(BasicInputLayout inputElementDescs, ComPtr<ID3DBlob> vertexShader, ComPtr<ID3DBlob> pixelShader, D3D12_DEPTH_STENCIL_DESC depthStencilDesc, ComPtr<ID3D12RootSignature> rootSignature);

	void CreateRootSignatureRTCP(UINT rootParamCount, UINT samplerCount, CD3DX12_ROOT_PARAMETER rootParameters[], CD3DX12_STATIC_SAMPLER_DESC samplers[], D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags, ComPtr<ID3D12RootSignature>& rootSignature);

	void InitShaderCompiler(D3D12ShaderCompilerInfo& shaderCompiler) const;

	void CreateTextureFromFileRTCP(ComPtr<ID3D12Resource>& texture, ComPtr<ID3D12GraphicsCommandList4> commandList, const wchar_t* path, ComPtr<ID3D12Resource>& uploadHeap, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES InitialResourceState);

	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, D3D12ShaderInfo& info, IDxcBlob** blob) const;
	void Compile_Shader(D3D12ShaderCompilerInfo& compilerInfo, RtProgram& program) const;
	void Compile_Shader(_In_ LPCWSTR pFileName, _In_reads_opt_(_Inexpressible_(pDefines->Name != NULL)) CONST D3D_SHADER_MACRO* pDefines, _In_opt_ ID3DInclude* pInclude, _In_ LPCSTR pEntrypoint, _In_ LPCSTR pTarget, _In_ UINT Flags1, _In_ UINT Flags2, _Out_ ID3DBlob** ppCode) const;

private:
	BasicInputLayout CreateBasicInputLayout();

	// Executing commands/synchronization functions
	void PopulateCommandList();
	void CloseCommandList();
	void WaitForPreviousFrame();
	void MoveToNextFrame();

private:
	static constexpr int m_frameCount = 2;

	XMFLOAT2 m_windowSize = XMFLOAT2{ 1280, 720 };

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
	ComPtr<ID3D12Resource> m_depthBuffer = NULL;

	// Textures
	ComPtr<ID3D12Resource> m_backBuffers[m_frameCount];

	// Root signatures/PSO
	ComPtr<ID3D12RootSignature> m_rootSignature = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineState = NULL;
	ComPtr<ID3D12RootSignature> m_rootSignatureSkybox = NULL;
	ComPtr<ID3D12PipelineState> m_pipelineStateSkybox = NULL;

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

	// Synchronization
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[m_frameCount];
	HANDLE m_fenceEvent;
};