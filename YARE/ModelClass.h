#pragma once

#include "pch.h"
#include <vector>
#include <array>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using namespace DirectX;

class ModelClass
{
public:
	ModelClass() = default;
	ModelClass(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList);
	ModelClass(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource>& uploadHeap);

	struct Texture {
		std::string type;
		std::string path;
		ComPtr<ID3D12Resource> resource;
		D3D12_SUBRESOURCE_DATA subresourceData;
		int textureID;
	};

	struct VertexBufferStruct {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT3 tangent;
		XMFLOAT3 binormal;
		XMFLOAT2 uv;
		unsigned int textureAlbedoID;
		unsigned int textureSpecRoughnessID;
		unsigned int textureNormalID;
	};

	struct Mesh {
		unsigned int vertexCount;
		unsigned int indexCount;

		std::vector<VertexBufferStruct> vertices;
		std::vector<unsigned long> indices;
	};

	// Creating or loading new model
	void SetFullScreenRectangleModel(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, float left = -1.0f, float right = 1.0f, float top = 1.0f, float bottom = -1.0f, DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);
	void LoadModel(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);
	void LoadModel(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource>& uploadHeap, DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT);
	void ProcessScene(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList);

	// Get meshes
	Mesh GetMesh(int index) const { return m_meshes.at(index); };
	std::vector<Mesh> GetMeshes() const { return m_meshes; };

	// Get textures
	std::vector<ComPtr<ID3D12Resource>>& GetTextureResourcesAlbedo() { return m_diffuseTexturesResources; };
	std::vector<ComPtr<ID3D12Resource>>& GetTextureResourcesSpecular() { return m_specularTexturesResources; };
	std::vector<ComPtr<ID3D12Resource>>& GetTextureResourcesNormal() { return m_normalTexturesResources; };

	// Get vertex buffer data
	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }
	ComPtr<ID3D12Resource> GetVertexBuffer() const { return m_vertexBuffer; }

	// Get index buffer data
	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_indexBufferView; }
	ComPtr<ID3D12Resource> GetIndexBuffer() const { return m_indexBuffer; }
	int GetIndicesCount() const { return m_indicesCount; }
	int GetVerticesCount() const { return m_verticesCount; }

private:
	// Processing data by assimp
	void ProcessNode(std::vector<Mesh>& meshes, aiNode* node, const aiScene* scene, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, unsigned int textureAlbedoID, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList);
	std::string DetermineTextureType(const aiScene* scene, aiMaterial* mat);
	std::vector<Texture> LoadMaterialTextures(ComPtr<ID3D12Resource>& resource, std::vector<Texture>& textures, aiMaterial* mat, std::string texType, aiTextureType type, std::string typeName, const aiScene* scene, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, int index, bool forceDDS = false);
	int GetTextureIndex(aiString* str);
	std::pair<ComPtr<ID3D12Resource>, D3D12_SUBRESOURCE_DATA> GetTextureFromModel(ComPtr<ID3D12Resource>& resource, const aiScene* scene, std::string filename, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, int index, bool forceDDS = false);

	// Internal functions - creating shapes
	bool CreateRectangle(ComPtr<ID3D12Device2> device, float left, float right, float top, float bottom);

	// Modifying vertex/index buffer, stored in class
	bool PrepareBuffers(ComPtr<ID3D12Device2> device, DXGI_FORMAT indexFormat);
	void UpdateBufferResource(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

//VARIABLES
private:
	// Resources storing textures
	//ComPtr<ID3D12Resource> m_resourceAlbedo;
	//ComPtr<ID3D12Resource> m_resourceSpecRoughness;
	std::vector<ComPtr<ID3D12Resource>> m_uploadHeaps{};
	std::vector<ComPtr<ID3D12Resource>> m_diffuseTexturesResources{};
	std::vector<ComPtr<ID3D12Resource>> m_specularTexturesResources{};
	std::vector<ComPtr<ID3D12Resource>> m_normalTexturesResources{};
	std::vector<Texture> m_diffuseTextures;
	std::vector<Texture> m_specularTextures;
	std::vector<Texture> m_normalTextures;

	// Stored meshes
	std::vector<Mesh> m_meshes;

	// Vertices and indices data
	ComPtr<ID3D12Resource> m_vertexBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_indexBuffer = NULL;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	int m_indicesCount = 0;
	int m_verticesCount = 0;
};