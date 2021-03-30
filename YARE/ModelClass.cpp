#include "ModelClass.h"
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <DirectXHelpers.h>
#include <DirectXTex.h>
#include <SimpleMath.h>
#include <regex>

ModelClass::ModelClass(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList)
{
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

	LoadModel(path, device, commandList);
}

ModelClass::ModelClass(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource>& uploadHeap)
{
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

	LoadModel(path, device, commandList, uploadHeap);
}

void ModelClass::LoadModel(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, DXGI_FORMAT indexFormat)
{
	ProcessScene(path, device, commandList);
	assert(PrepareBuffers(device, indexFormat) && "Failed to prepare buffers");
}

void ModelClass::LoadModel(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ComPtr<ID3D12Resource>& uploadHeap, DXGI_FORMAT indexFormat)
{
	Assimp::Importer importer;
	char result[MAX_PATH];
	const std::string filename = path;
	path = std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
	const auto index = path.find_last_of('\\');
	path = path.substr(0, index + 1) + filename;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | /*aiProcess_ConvertToLeftHanded | */ aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

	assert(scene);
#pragma warning(push)
#pragma warning(disable : 6011)
	//ProcessNode(scene->mRootNode, scene);
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		m_meshes.push_back(ProcessMesh(scene->mMeshes[i], scene, i, device, commandList));
	}
#pragma warning(pop)

	assert(PrepareBuffers(device, indexFormat) && "Failed to prepare buffers");
}

void ModelClass::ProcessScene(std::string path, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	Assimp::Importer importer;
	char result[MAX_PATH];
	const std::string filename = path;
	path = std::string(result, GetModuleFileNameA(NULL, result, MAX_PATH));
	const auto index = path.find_last_of('\\');
	path = path.substr(0, index + 1) + filename;

	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

	assert(scene);
#pragma warning(push)
#pragma warning(disable : 6011)
	//ProcessNode(scene->mRootNode, scene);
	for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
	{
		m_meshes.push_back(ProcessMesh(scene->mMeshes[i], scene, i, device, commandList));
	}
#pragma warning(pop)
}

void ModelClass::ProcessNode(std::vector<Mesh>& meshes, aiNode* node, const aiScene* scene, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	for (unsigned int i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(ProcessMesh(mesh, scene, i, device, commandList));
	}

	for (unsigned int i = 0; i < node->mNumChildren; ++i)
	{
		ProcessNode(meshes, node->mChildren[i], scene, device, commandList);
	}
}

ModelClass::Mesh ModelClass::ProcessMesh(aiMesh* mesh, const aiScene* scene, unsigned int textureID, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList)
{
	Mesh localMesh;
	localMesh.vertices.resize(mesh->mNumVertices);
	bool hasTexAlbedo = false;
	bool hasTexSpecRoughness = false;
	bool hasTexNormal = false;
	unsigned int albedoTexID = -1;
	unsigned int specRoughnessTexID = -1;
	unsigned int normalTexID = -1;

	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

		m_diffuseTexturesResources.push_back({});
		std::vector<Texture> diffuseMaps = LoadMaterialTextures(m_diffuseTexturesResources[textureID], m_diffuseTextures, material, DetermineTextureType(scene, material), aiTextureType_DIFFUSE, "texture_diffuse", scene, device, commandList, textureID);
		if (diffuseMaps.size() > 0)
		{
			hasTexAlbedo = true;
			albedoTexID = diffuseMaps[0].textureID;
		}

		m_specularTexturesResources.push_back({});
		std::vector<Texture> specularMaps = LoadMaterialTextures(m_specularTexturesResources[textureID], m_specularTextures, material, DetermineTextureType(scene, material), aiTextureType_SPECULAR, "texture_specRoughness", scene, device, commandList, textureID);
		if (specularMaps.size() > 0)
		{
			hasTexSpecRoughness = true;
			specRoughnessTexID = specularMaps[0].textureID;
		}

		m_normalTexturesResources.push_back({});
		std::vector<Texture> normalMaps = LoadMaterialTextures(m_normalTexturesResources[textureID], m_normalTextures, material, DetermineTextureType(scene, material), aiTextureType_NORMALS, "texture_normal", scene, device, commandList, textureID, true);
		if (normalMaps.size() > 0)
		{
			hasTexNormal = true;
			normalTexID = normalMaps[0].textureID;
		}
	}

	for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
	{
		//POSITION
		VertexBufferStruct vertex;
		vertex.position = XMFLOAT3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

		// TEXTURE ID
		//vertex.textureAlbedoID = hasTexAlbedo ? albedoTexID : -1;
		//vertex.textureSpecRoughnessID = hasTexSpecRoughness ? specRoughnessTexID : -1;
		//vertex.textureNormalID = hasTexNormal ? normalTexID : -1;

		//NORMAL
		if (mesh->mNormals) {
			vertex.normal = XMFLOAT3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
		}
		else {
			assert(false && "Mesh doesn't have normals, which is not supported");
		}

		//UV
		if (mesh->HasTextureCoords(0)) {
			vertex.uv.x = mesh->mTextureCoords[0][i].x;
			vertex.uv.y = mesh->mTextureCoords[0][i].y;
		}
		else {
			vertex.uv = XMFLOAT2{ 0,0 };
		}

		//TANGENT and BINORMAL
		if (!mesh->mBitangents || !mesh->mTangents)
		{
			if (i >= 2 && (i - 2) % 3 == 0)
			{
				// https://learnopengl.com/Advanced-Lighting/Normal-Mapping
				const XMFLOAT3 pos1 = localMesh.vertices[i - 2].position;
				const XMFLOAT3 pos2 = localMesh.vertices[i - 1].position;
				const XMFLOAT3 pos3 = localMesh.vertices[i].position;
				const XMFLOAT3 edge1 = XMFLOAT3{ pos2.x - pos1.x, pos2.y - pos1.y, pos2.z - pos1.z };
				const XMFLOAT3 edge2 = XMFLOAT3{ pos3.x - pos1.x, pos3.y - pos1.y, pos3.z - pos1.z };

				const XMFLOAT2 uv1 = localMesh.vertices[i - 2].uv;
				const XMFLOAT2 uv2 = localMesh.vertices[i - 1].uv;
				const XMFLOAT2 uv3 = localMesh.vertices[i].uv;
				const XMFLOAT2 deltaUV1 = XMFLOAT2{ uv2.x - uv1.x, uv2.y - uv1.y };
				const XMFLOAT2 deltaUV2 = XMFLOAT2{ uv3.x - uv1.x, uv3.y - uv1.y };

				const float denominator = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

				XMVECTOR tangent = XMVectorSet(denominator * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
					denominator * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
					denominator * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z),
					0.0f);
				tangent = XMVector3Normalize(tangent);

				XMVECTOR binormal = XMVectorSet(denominator * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x),
					denominator * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y),
					denominator * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z),
					0.0f);
				binormal = XMVector3Normalize(binormal);

				localMesh.vertices[i].tangent = XMFLOAT3{ tangent.m128_f32[0], tangent.m128_f32[1], tangent.m128_f32[2] };
				localMesh.vertices[i].binormal = XMFLOAT3{ binormal.m128_f32[0], binormal.m128_f32[1], binormal.m128_f32[2] };
			}
		}
		else
		{
			if (mesh->mBitangents) {
				vertex.binormal = XMFLOAT3{ mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z };
			}

			if (mesh->mTangents) {
				vertex.tangent = XMFLOAT3{ mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
			}
		}

		//Store in array
		localMesh.vertices[i] = vertex;
	}

	unsigned int indicesCount = 0;
	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		indicesCount += face.mNumIndices;
	}
	localMesh.indices.resize(indicesCount);
	localMesh.indexCount = indicesCount;
	localMesh.vertexCount = mesh->mNumVertices;
	size_t currentIndexCounter = 0;

	for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; ++j)
		{
			localMesh.indices[currentIndexCounter++] = static_cast<unsigned long>(face.mIndices[j]);
		}
	}

	return localMesh;
}

std::string ModelClass::DetermineTextureType(const aiScene* scene, aiMaterial* mat)
{
	aiString texTypeString;
	mat->GetTexture(aiTextureType_DIFFUSE, 0, &texTypeString);
	std::string texTypeTestStr = texTypeString.C_Str();
	if (texTypeTestStr == "*0" || texTypeTestStr == "*1" || texTypeTestStr == "*2" || texTypeTestStr == "*3" || texTypeTestStr == "*4" || texTypeTestStr == "*5") {
		if (scene->mTextures[0]->mHeight == 0) {
			return "embedded compressed texture";
		}
		else {
			return "embedded non-compressed texture";
		}
	}
	if (texTypeTestStr.find('.') != std::string::npos) {
		return "textures are on disk";
	}

	return ".";
}

std::vector<ModelClass::Texture> ModelClass::LoadMaterialTextures(ComPtr<ID3D12Resource>& resource, std::vector<Texture>& texturesRef, aiMaterial* mat, std::string texType, aiTextureType type, std::string typeName, const aiScene* scene, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, int index, bool forceDDS /* = false */)
{
	std::vector<Texture> textures;

	for (UINT i = 0; i < mat->GetTextureCount(type); ++i)
	{
		aiString str;
		mat->GetTexture(type, i, &str);
		// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
		bool skip = false;
		for (UINT j = 0; j < texturesRef.size(); j++) {
			if (std::strcmp(texturesRef[j].path.c_str(), str.C_Str()) == 0) {
				textures.push_back(texturesRef[j]);
				textures[textures.size() - 1].textureID = texturesRef[j].textureID;
				skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)
				break;
			}
		}

		if (!skip) {   // If texture hasn't been loaded already, load it
			Texture texture;
			if (texType == "embedded compressed texture") {
				//int textureindex = GetTextureIndex(&str);
				//texture.resource = GetTextureFromModel(scene, textureindex, device, commandList);
			}
			else
			{
				//int textureindex = GetTextureIndex(&str);
				std::string filename = std::string(str.C_Str());
				auto data = GetTextureFromModel(resource, scene, filename, device, commandList, index, forceDDS);
				texture.resource = data.first;
				texture.subresourceData = data.second;
			}
			texture.textureID = index;
			texture.type = typeName;
			texture.path = str.C_Str();
			textures.push_back(texture);
			texturesRef.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
		}
	}

	return textures;
}

int ModelClass::GetTextureIndex(aiString* str)
{
	std::string tistr;
	tistr = str->C_Str();
	tistr = tistr.substr(1);
	return stoi(tistr);
}

std::pair<ComPtr<ID3D12Resource>, D3D12_SUBRESOURCE_DATA> ModelClass::GetTextureFromModel(ComPtr<ID3D12Resource>& resource, const aiScene* scene, std::string filename, ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, int index, bool forceDDS /* = false */)
{
	std::vector<D3D12_SUBRESOURCE_DATA> textureData;
	D3D12_SUBRESOURCE_DATA textureDataSingle;
	std::unique_ptr<uint8_t[]> decodedData;
	ComPtr<ID3D12Resource> texture;
	m_uploadHeaps.push_back({});

	std::string s = std::regex_replace(filename, std::regex("\\\\"), "/");

	std::wstring ws(s.begin(), s.end());
	if (forceDDS) {
		ThrowIfFailed(LoadDDSTextureFromFileEx(device.Get(), ws.c_str(), 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, texture.ReleaseAndGetAddressOf(), decodedData, textureData));
		textureDataSingle = textureData[0];
	}
	else {
		ThrowIfFailed(LoadWICTextureFromFileEx(device.Get(), ws.c_str(), 0, D3D12_RESOURCE_FLAG_NONE, WIC_LOADER_DEFAULT, texture.ReleaseAndGetAddressOf(), decodedData, textureDataSingle));
	}

	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture.Get(), 0, 1);
	//auto desc = texture->GetDesc();

	// uploadHeap must outlive this function - until command list is closed
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_uploadHeaps[m_uploadHeaps.size() - 1])
	));

	UpdateSubresources(commandList.Get(), texture.Get(), m_uploadHeaps[m_uploadHeaps.size() - 1].Get(), 0, 0, 1, &textureDataSingle);
	//commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, InitialResourceState));

	//if (textureDataSingle.SlicePitch == 128 * 128 * 4)
	//if (texture->GetDesc().Width == 128 && texture->GetDesc().Height == 128)
	{
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE));

		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		/*		if (texture->GetDesc().Format == DXGI_FORMAT_BC1_UNORM  || texture->GetDesc().Format == DXGI_FORMAT_BC1_TYPELESS) {
					textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				}
				else if (texture->GetDesc().Format == DXGI_FORMAT_BC3_UNORM || texture->GetDesc().Format == DXGI_FORMAT_BC3_TYPELESS || texture->GetDesc().Format == DXGI_FORMAT_BC5_UNORM) {
					textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				}
				else */ {
			textureDesc.Format = texture->GetDesc().Format; /*DXGI_FORMAT_R8G8B8A8_UNORM*/;
		}
				textureDesc.Width = texture->GetDesc().Width;
				textureDesc.Height = texture->GetDesc().Height;
				textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				textureDesc.DepthOrArraySize = 1;
				textureDesc.SampleDesc.Count = 1;
				textureDesc.SampleDesc.Quality = 0;
				textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

				ThrowIfFailed(device->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
					D3D12_HEAP_FLAG_NONE,
					&textureDesc,
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(&resource)
				));

				D3D12_TEXTURE_COPY_LOCATION dst{};
				dst.pResource = resource.Get();

				D3D12_TEXTURE_COPY_LOCATION src{};
				src.pResource = texture.Get();

				commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
	}

	return { texture, textureDataSingle };
}

bool ModelClass::CreateRectangle(ComPtr<ID3D12Device2> device, float left, float right, float top, float bottom)
{
	Mesh localMesh;
	constexpr size_t verticesCount = 6;

	localMesh.vertices.resize(verticesCount);
	localMesh.indices.resize(verticesCount);
	localMesh.vertexCount = verticesCount;
	localMesh.indexCount = verticesCount;
	for (size_t i = 0; i < verticesCount; ++i)
	{
		localMesh.indices[i] = static_cast<unsigned long>(i);
	}

	//First triangle
	localMesh.vertices[0].position = XMFLOAT3(left, top, 0.0f);  // Top left.	
	localMesh.vertices[1].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	localMesh.vertices[2].position = XMFLOAT3(left, bottom, 0.0f);  // Bottom left.
	//Second triangle
	localMesh.vertices[3].position = XMFLOAT3(left, top, 0.0f);  // Top left.
	localMesh.vertices[4].position = XMFLOAT3(right, top, 0.0f);  // Top right.
	localMesh.vertices[5].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.

	localMesh.vertices[0].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[1].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[2].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[3].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[4].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);
	localMesh.vertices[5].normal = XMFLOAT3(0.0f, 0.0f, -1.0f);

	//Set UV values
	{
		//First triangle	
		localMesh.vertices[0].uv = XMFLOAT2(0.0, 0.0);  // Top left.	
		localMesh.vertices[1].uv = XMFLOAT2(1.0, 1.0);  // Bottom right.
		localMesh.vertices[2].uv = XMFLOAT2(0.0, 1.0);  // Bottom left.
		//Second triangle
		localMesh.vertices[3].uv = XMFLOAT2(0.0, 0.0);  // Top left.
		localMesh.vertices[4].uv = XMFLOAT2(1.0, 0.0);  // Top right.
		localMesh.vertices[5].uv = XMFLOAT2(1.0, 1.0);  // Bottom right.
	}

	m_meshes.clear();
	m_meshes.push_back(localMesh);

	return true;
}

bool ModelClass::PrepareBuffers(ComPtr<ID3D12Device2> device, DXGI_FORMAT indexFormat)
{
	std::vector<ModelClass::VertexBufferStruct> vertices{};
	std::vector<unsigned long> indices{};

	for (Mesh mesh : m_meshes)
	{
		std::copy(mesh.vertices.begin(), mesh.vertices.end(), back_inserter(vertices));
		//std::copy(mesh.indices.begin(), mesh.indices.end(), back_inserter(indices));
		unsigned long count = static_cast<unsigned long>(indices.size());
		for (unsigned long i : mesh.indices)
		{
			indices.push_back(i + count);
		}
	}

	// Create vertex buffer
	{
		const UINT vertexBufferSize = static_cast<UINT>(sizeof(ModelClass::VertexBufferStruct) * static_cast<UINT>(vertices.size()));
		m_verticesCount = static_cast<UINT>(vertices.size());

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, &vertices[0], vertexBufferSize);
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(ModelClass::VertexBufferStruct);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create index buffer
	{
		UINT indicesByteMultiplier = 0;
		if (indexFormat == DXGI_FORMAT_R32_UINT) {
			indicesByteMultiplier = 4;
		}
		else if (indexFormat == DXGI_FORMAT_R16_UINT) {
			indicesByteMultiplier = 2;
		}
		else
		{
			assert(false && "Unrecognized DXGI_FORMAT - only R16_UINT and R32_UINT are supported for index buffer in this project");
			return false;
		}

		const UINT indexBufferSize = static_cast<UINT>(indices.size()) * indicesByteMultiplier;
		m_indicesCount = static_cast<UINT>(indices.size());

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pIndexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
		memcpy(pIndexDataBegin, &indices[0], indexBufferSize);
		m_indexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = indexFormat;
		m_indexBufferView.SizeInBytes = indexBufferSize;
	}

	return true;
}

void ModelClass::UpdateBufferResource(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	size_t bufferSize = numElements * elementSize;
	{
		ThrowIfFailed(
			device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(pDestinationResource))
		);
	}

	// Make sure that passed data is not empty
	if (bufferData)
	{
		ThrowIfFailed(
			device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(pIntermediateResource))
		);

		D3D12_SUBRESOURCE_DATA subresourceData{};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
	}
}

void ModelClass::SetFullScreenRectangleModel(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList4> commandList, float left, float right, float top, float bottom, DXGI_FORMAT indexFormat)
{
	assert(CreateRectangle(device, left, right, top, bottom) && "Failed to create full screen rectangle");
	assert(PrepareBuffers(device, indexFormat) && "Failed to prepare buffers");
}
