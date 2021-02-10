#ifndef _RT_AO_HLSL_
#define _RT_AO_HLSL_

#include "RT_HelperFunctions.hlsl"
#include "UniformDistribution.hlsl"
#include "SamplingHelper.hlsl"
#include "ALL_CommonBuffers.hlsl"

// Structures
struct RayPayload
{
	float T;
};

////

struct RaytracingAOBuffer
{
	float aoRadius;
	float minT;
	int accFrames;
	int sampleCount;
	int maxFrames;
	
	float padding[59];
};

// Resources
ConstantBuffer<SceneConstantBuffer> g_sceneCB : register(b0);
ConstantBuffer<CameraConstantBuffer> g_cameraCB : register(b1);
ConstantBuffer<RaytracingAOBuffer> g_aoCB : register(b2);

RWTexture2D<float4> RTOutput : register(u0);

RaytracingAccelerationStructure SceneBVH : register(t0);
ByteAddressBuffer indices : register(t1);
StructuredBuffer<Vertex> vertices : register(t2);
Texture2D<float4> NormalTextureInput : register(t3);
Texture2D<float4> albedoTexture : register(t4);

////

inline void GenerateCameraRayAO(uint2 index, out float3 origin, out float3 direction)
{
	float2 xy = index + g_cameraCB.jitterCamera; // center in the middle of the pixel.
	float2 screenPos = (xy / DispatchRaysDimensions().xy) * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
	//screenPos.y = -screenPos.y;
	screenPos *= float2(0.5 * 16.0f / 9.0f, 0.5);

    // Unproject the pixel coordinate into a ray.
	float4 world = mul(float4(screenPos, 0, 1), g_sceneCB.projectionToWorld);

	world.xyz /= world.w;
	direction = screenPos.x * g_cameraCB.cameraU.xyz + screenPos.y * g_cameraCB.cameraV.xyz + g_cameraCB.cameraW.xyz;
	direction /= length(g_cameraCB.cameraW.xyz);
	
	// Calculate random ray origin
	float3 focalPoint = g_sceneCB.cameraPosition.xyz + g_cameraCB.focalLength * direction;
	
	uint randSeed = initRand(index.x + index.y * DispatchRaysDimensions().x, g_sceneCB.frameCount);
	float2 rnd = float2(nextRand(randSeed) * PI * 2.0, nextRand(randSeed) * g_cameraCB.lensRadius);
	float2 uv = float2(cos(rnd.x) * rnd.y, sin(rnd.x) * rnd.y);
	
	origin = g_sceneCB.cameraPosition.xyz + uv.x * normalize(g_cameraCB.cameraU.xyz) + uv.y * normalize(g_cameraCB.cameraV.xyz);
}

[shader("raygeneration")]
void RayGen()
{
	uint2 LaunchIndex = DispatchRaysIndex().xy;
	uint2 LaunchDimensions = DispatchRaysDimensions().xy;

	float4 normalAndDepth = NormalTextureInput.Load(int3(LaunchIndex, 0));
	
	if (normalAndDepth.w == 0)
	{
        // Terminate if primary ray in g-buffer pass didn't hit anything
		RTOutput[LaunchIndex] = float4(1.0f, 1.0f, 1.0f, 1.0f);
		return;
	}
	
	// Find primary ray origin and direction
	float3 primaryRayOrigin = g_sceneCB.cameraPosition.xyz;
	float3 primaryRayDirection;
	GenerateCameraRay(LaunchIndex, LaunchDimensions, g_sceneCB.projectionToWorld, primaryRayOrigin, primaryRayDirection);
	
	// Calculate direction of raytracing for AO sample
	float3 pixelWorldSpacePosition = primaryRayOrigin + (primaryRayDirection * normalAndDepth.w);
	uint seed = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_sceneCB.frameCount);
	
	float ao = 0.0f;
	for (int i = 0; i < g_aoCB.sampleCount; i++)
	{
		nextRand(seed);
		float3 worldDir = getCosHemisphereSample(seed.x, normalAndDepth.xyz);
	
		// Prepare ray description and payload
		RayDesc aoRay = { pixelWorldSpacePosition, g_aoCB.minT, worldDir, g_aoCB.aoRadius };
		RayPayload payload;
		// Set T to "maximum", no occlusion if ray doesn't hit anything 
		payload.T = g_aoCB.aoRadius;

		// Trace the ray
		TraceRay(SceneBVH, RAY_FLAG_NONE, 0xFF, 0, 1, 0, aoRay, payload);
		ao += payload.T / g_aoCB.aoRadius;
	}
	ao /= (float) g_aoCB.sampleCount;
	// Accumulate AO with previous frames
	float prevAO = RTOutput[LaunchIndex].x;
	ao = ((float) g_aoCB.accFrames * prevAO + ao) / ((float) g_aoCB.accFrames + 1.0f);
	RTOutput[LaunchIndex.xy] = float4(ao, ao, ao, 1.0f);
}

[shader("miss")]
void Miss(inout RayPayload payload : SV_RayPayload)
{
	// Empty
}

[shader("closesthit")]
void ClosestHit(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.T = RayTCurrent();
}

#endif //_RT_AO_HLSL_