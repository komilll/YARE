#ifndef _RT_G_BUFFER_HLSL_
#define _RT_G_BUFFER_HLSL_

#include "RT_HelperFunctions.hlsl"
#include "UniformDistribution.hlsl"
#include "SamplingHelper.hlsl"
#include "ALL_CommonBuffers.hlsl"

struct RayPayload
{
	bool miss;
};

////

// Resources
ConstantBuffer<SceneConstantBuffer> g_sceneCB : register(b0);
ConstantBuffer<CameraConstantBuffer> g_cameraCB : register(b1);

RWTexture2D<float4> RTOutputNormal : register(u0);
RWTexture2D<float4> RTOutputAlbedo : register(u1);
RWTexture2D<float4> RTOutputSpecRoughness : register(u2);

RaytracingAccelerationStructure SceneBVH : register(t0);
ByteAddressBuffer indices : register(t1);
StructuredBuffer<Vertex> vertices : register(t2);
TextureCube<float4> skyboxTexture : register(t3);
Texture2D<float4> textures[] : register(t4);
//Texture2DArray<float4> specularRoughnessTex : register(t4);

SamplerState g_sampler : register(s0);

[shader("raygeneration")]
void RayGen()
{
	float3 rayDir;
	float3 origin = g_sceneCB.cameraPosition.xyz;
    
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
	GenerateCameraRay(DispatchRaysIndex().xy, DispatchRaysDimensions().xy, g_sceneCB.projectionToWorld, origin, rayDir);

	RayDesc ray = { origin, 1e-4f, rayDir, 1e+38f };
	RayPayload payload = { false };
	TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, ray, payload);
	
	if (payload.miss)
	{
		rayDir.z = -rayDir.z;
	
		RTOutputAlbedo[DispatchRaysIndex().xy] = float4(skyboxTexture.SampleLevel(g_sampler, rayDir, 0).rgb, 0);
		RTOutputNormal[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
		RTOutputSpecRoughness[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
	}
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	uint indexSizeInBytes = 4;
	uint indicesPerTriangle = 3;
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
	uint baseIndex = PrimitiveIndex() * triangleIndexStride;
	
	const uint3 indices_ = indices.Load3(baseIndex);

	float3 vertexNormals[3] =
	{
		vertices[indices_[0]].normal,
		vertices[indices_[1]].normal,
		vertices[indices_[2]].normal
	};
	
	float3 triangleNormal = HitAttribute(vertexNormals, attribs);
	
	RTOutputNormal[DispatchRaysIndex().xy] = float4(triangleNormal, RayTCurrent());
	
	float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
	float2 vertexUVs[3] =
	{
		vertices[indices_[0]].uv,
		vertices[indices_[1]].uv,
		vertices[indices_[2]].uv
	};
	
	float2 uv = barycentrics.x * vertexUVs[0] + barycentrics.y * vertexUVs[1] + barycentrics.z * vertexUVs[2];
	RTOutputAlbedo[DispatchRaysIndex().xy] = float4(textures[vertices[indices_[0]].textureAlbedoID].SampleLevel(g_sampler, uv, 0).xyz, 1);
	RTOutputSpecRoughness[DispatchRaysIndex().xy] = float4(textures[vertices[indices_[0]].textureSpecRoughnessID + g_sceneCB.specularIndex].SampleLevel(g_sampler, uv, 0).xyz, 1);
	RTOutputNormal[DispatchRaysIndex().xy] = float4(textures[vertices[indices_[0]].textureNormalID + g_sceneCB.normalIndex].SampleLevel(g_sampler, uv, 0).xyz, 1);
}

[shader("miss")]
void Miss(inout RayPayload payload)
{
	payload.miss = true;
	//float3 rayDir = WorldRayDirection();
	//rayDir.z = -rayDir.z;
	
	//RTOutputAlbedo[DispatchRaysIndex().xy] = float4(skyboxTexture.SampleLevel(g_sampler, rayDir, 0).rgb, 0);
	//RTOutputNormal[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
	//RTOutputSpecRoughness[DispatchRaysIndex().xy] = float4(0, 0, 0, 0);
}

#endif //_RT_G_BUFFER_HLSL_