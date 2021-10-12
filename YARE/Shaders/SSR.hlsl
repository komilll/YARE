#ifndef _SSR_HLSL
#define _SSR_HLSL

#include "ALL_CommonBuffers.hlsl"

SamplerState pointSampler : register(s0);

Texture2D hiZBuffer : register(t0);
Texture2D visibilityBuffer : register(t1);
Texture2D normalBuffer : register(t2);
Texture2D colorBuffer : register(t3);

RWTexture2D<float> visibilityOut : register(u0);

ConstantBuffer<MipConstantBuffer> g_mipCB : register(b0);
ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b1);
cbuffer g_preIntegrateCB : register(b2)
{
	float zNear;
	float zFar;
	float preIntegratePadding[62];
};

ConstantBuffer<SceneConstantBuffer> g_constantBuffer : register(b3);

float depthVStoCS(float depthVS)
{
	return (g_matricesCB.projMatrix._43 + depthVS * g_matricesCB.projMatrix._33) / (depthVS * g_matricesCB.projMatrix._34);
}

float linearize(float depthVal)
{
	//float val = (2.0f * zNear) / (zFar + zNear - depthVal * (zFar - zNear)); // [-1, 1]
	//val = (val + 1.0f) * 0.5f; // [0, 1]
	//return val;
	
	float z = depthVStoCS(depthVal);
	float val = (2.0f * zNear) / (zFar + zNear - z * (zFar - zNear));
	return val;
}

#define max3(x, y, z)       ( max(max(x, y), z) )
#define min3(x, y, z)       ( min(min(x, y), z) )
#define min4(x, y, z, w)    ( min( min3(x, y, z), w) )
#define max4(x, y, z, w)    ( max( max3(x, y, z), w) )

[numthreads(16, 16, 1)]
void preIntegrateMip0(uint3 index : SV_DispatchThreadID)
{
	float2 texcoords = float2((float) index.x / 512.0f, (float) index.y / 256.0f);
	
	int mipCurrent = 1;
	int mipPrevious = 0;
	
	float4 fineZ;
	fineZ.x = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 0)).y);
	fineZ.y = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 0)).y);
	fineZ.z = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 1)).y);
	fineZ.w = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 1)).y);
	
	float maxZ = max4(fineZ.x, fineZ.y, fineZ.z, fineZ.w);
	
	// Visibility of mip0 is always 1.0
	float4 integration = fineZ.xyzw / maxZ; //* visibility.xyzw;
	float coarseIntegration = dot(0.25f, integration.xyzw);
	
	visibilityOut[index.xy] = coarseIntegration;
}

[numthreads(16, 16, 1)]
void preIntegrate(uint3 index : SV_DispatchThreadID)
{
	float2 texcoords = float2((float) index.x / 256.0f, (float) index.y / 128.0f);
	
	int mipCurrent = 2;
	int mipPrevious = 1;
	
	float4 fineZ;
	fineZ.x = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 0)).y);
	fineZ.y = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 0)).y);
	fineZ.z = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 1)).y);
	fineZ.w = linearize(hiZBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 1)).y);
	
	float maxZ = max4(fineZ.x, fineZ.y, fineZ.z, fineZ.w);
	
	float4 visibility;
	visibility.x = visibilityBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 0)).x;
	visibility.y = visibilityBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 0)).x;
	visibility.z = visibilityBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(0, 1)).x;
	visibility.w = visibilityBuffer.SampleLevel(pointSampler, texcoords, mipPrevious, int2(1, 1)).x;
	
	float4 integration = fineZ.xyzw / maxZ * visibility.xyzw;
	float coarseIntegration = dot(0.25f, integration.xyzw);
	
	visibilityOut[index.xy] = coarseIntegration;
}

static const float HIZ_START_LEVEL = 2.0f;
static const float HIZ_STOP_LEVEL = 2.0f;
static const float HIZ_MAX_LEVEL = 3.0f;
static const float2 HIZ_CROSS_EPSILON = float2(1.0f / 1024.0f, 1.0f / 512.0f); // maybe need to be smaller or larger? this is mip level 0 texel size
static const uint MAX_ITERATIONS = 64u;

float3 intersectDepthPlane(float3 o, float3 d, float t)
{
	return o + d * t;
}

float2 getCell(float2 ray, float2 cellCount)
{
	// does this need to be floor, or does it need fractional part - i think cells are meant to be whole pixel values (integer values) but not sure
	return floor(ray * cellCount);
}

float3 intersectCellBoundary(float3 o, float3 d, float2 cellIndex, float2 cellCount, float2 crossStep, float2 crossOffset)
{
	float2 index = cellIndex + crossStep;
	index /= cellCount;
	index += crossOffset;
	float2 delta = index - o.xy;
	delta /= d.xy;
	float t = min(delta.x, delta.y);
	return intersectDepthPlane(o, d, t);
}

float getMinimumDepthPlane(float2 ray, float level, float rootLevel)
{
	// not sure why we need rootLevel for this
	return hiZBuffer.SampleLevel(pointSampler, ray.xy, level).r;
}

float2 getCellCount(float level, float rootLevel)
{
	// not sure why we need rootLevel for this
	float2 div = level == 0.0f ? 1.0f : exp2(level);
	return float2(1024.0f, 512.0f) / div;
}

bool crossedCellBoundary(float2 cellIdxOne, float2 cellIdxTwo)
{
	return cellIdxOne.x != cellIdxTwo.x || cellIdxOne.y != cellIdxTwo.y;
}

float3 hiZTrace(float3 p, float3 v)
{
	const float rootLevel = 2.0f;
	
	float level = 0.0f;
	
	uint iterations = 0u;
	
	// get the cell cross direction and a small offset to enter the next cell when doing cell crossing
	float2 crossStep = float2(v.x >= 0.0f ? 1.0f : -1.0f, v.y >= 0.0f ? 1.0f : -1.0f);
	float2 crossOffset = float2(crossStep.xy * HIZ_CROSS_EPSILON.xy);
	crossStep.xy = saturate(crossStep.xy);
	
	// set current ray to original screen coordinate and depth
	float3 ray = p.xyz;
	
	// scale vector such that z is 1.0f (maximum depth)
	float3 d = v.xyz / v.z;
	
	// set starting point to the point where z equals 0.0f (minimum depth)
	float3 o = intersectDepthPlane(p, d, -p.z);
	
	// cross to next cell to avoid immediate self-intersection
	float2 rayCell = getCell(ray.xy, float2(1024.0f, 512.0f));
	ray = intersectCellBoundary(o, d, rayCell.xy, float2(1024.0f, 512.0f), crossStep.xy, crossOffset.xy);
	
	while (level >= HIZ_STOP_LEVEL && iterations < MAX_ITERATIONS)
	{
		// get the minimum depth plane in which the current ray resides
		float minZ = getMinimumDepthPlane(ray.xy, level, rootLevel);
		
		// get the cell number of the current ray
		const float2 cellCount = getCellCount(level, rootLevel);
		const float2 oldCellIdx = getCell(ray.xy, cellCount);

		// intersect only if ray depth is below the minimum depth plane
		float3 tmpRay = intersectDepthPlane(o, d, max(ray.z, minZ));

		// get the new cell number as well
		const float2 newCellIdx = getCell(tmpRay.xy, cellCount);

		// if the new cell number is different from the old cell number, a cell was crossed
		if (crossedCellBoundary(oldCellIdx, newCellIdx))
		{
			// intersect the boundary of that cell instead, and go up a level for taking a larger step next iteration
			tmpRay = intersectCellBoundary(o, d, oldCellIdx, cellCount.xy, crossStep.xy, crossOffset.xy); //// NOTE added .xy to o and d arguments
			level = min(HIZ_MAX_LEVEL, level + 2.0f);
		}

		ray.xyz = tmpRay.xyz;

		// go down a level in the hi-z buffer
		--level;

		++iterations;
	}

	return ray;
}

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD0;
	//uint textureID : TEXCOORD1;
};

float3 ProjectPoint(float3 viewPoint)
{
	float4 projPoint = mul(g_matricesCB.projMatrix, float4(viewPoint, 1.0));
	projPoint.xyz /= projPoint.w;
	//projPoint.xy = projPoint.xy * float2(0.5, -0.5) + float2(0.5f, 0.5f);
	return projPoint.xyz;
}

float3 UnprojectPoint(float3 screenPoint)
{
	//screenPoint.xy = (screenPoint.xy - float2(0.5f, 0.5f)) * float2(2.0, -2.0);
	float4 projPos = mul(g_matricesCB.invProjMatrix, float4(screenPoint, 1.0f));
	return projPos.xyz / projPos.w;
}

float3 positionWSFromDepthCS(float depthCS, uint2 positionCS)
{
	float4 positionWS = mul(float4(positionCS, depthCS, 1.0f), g_matricesCB.invViewProjMatrix);
	return positionWS.xyz / positionWS.w;
}

float3 getViewVector(float3 positionWS)
{
	return g_constantBuffer.cameraPosition.xyz - positionWS;
}

#define MAX_REFLECTION_RAY_MARCH_STEP 0.02f
#define NUM_RAY_MARCH_SAMPLES 16

bool GetReflection(
	float3 ScreenSpaceReflectionVec,
	float3 ScreenSpacePos,
	out float3 ReflectionColor)
{
	// Raymarch in the direction of the ScreenSpaceReflectionVec until you get an intersection with your z buffer
	for (int RayStepIdx = 0; RayStepIdx < NUM_RAY_MARCH_SAMPLES; RayStepIdx++)
	{
		float3 RaySample = (RayStepIdx * MAX_REFLECTION_RAY_MARCH_STEP) * ScreenSpaceReflectionVec + ScreenSpacePos;
		float ZBufferVal = depthVStoCS(hiZBuffer.Sample(pointSampler, RaySample.xy).r);
				
		if (RaySample.z > ZBufferVal)
		{
			ReflectionColor = colorBuffer.SampleLevel(pointSampler, RaySample.xy, 0).rgb;
			return true;
		}
	}

	return false;
}

float4 SSR(PixelInputType input) : SV_TARGET
{
	float3 normal = normalBuffer.Sample(pointSampler, input.uv).rgb;
	
	if (dot(normal, float3(1.0f, 1.0f, 1.0f)) == 0.0f)
	{
		return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	
	float2 PixelUV = input.uv;
	float2 NDCPos = float2(2.0f, -2.0f) * PixelUV + float2(-1.0f, 1.0f);
	
	// Prerequisites
	float DeviceZ = linearize(depthVStoCS(hiZBuffer.Sample(pointSampler, input.uv).r));
	float3 WorldPosition = positionWSFromDepthCS(DeviceZ, input.uv);
	WorldPosition = mul(float4(WorldPosition, 1.0f), g_matricesCB.viewMatrix);
	float3 CameraVector = normalize(WorldPosition - g_constantBuffer.cameraPosition.xyz);
	float4 WorldNormal = float4(normal, 1.0f);// * float4(2, 2, 2, 1) - float4(1, 1, 1, 0);
	WorldNormal = mul(WorldNormal, g_matricesCB.viewMatrix);
	
	// ScreenSpacePos --> (screencoord.xy, device_z)
	float4 ScreenSpacePos = float4(PixelUV, DeviceZ, 1.f);
	
	// Compute world space reflection vector
	float3 ReflectionVector = normalize(reflect(CameraVector, WorldNormal.xyz));
	
	// Compute second sreen space point so that we can get the SS reflection vector
	float4 PointAlongReflectionVec = float4(ReflectionVector + WorldPosition, 1.f);
	float4 ScreenSpaceReflectionPoint = mul(g_matricesCB.projMatrix, PointAlongReflectionVec);
	ScreenSpaceReflectionPoint /= ScreenSpaceReflectionPoint.w;
	ScreenSpaceReflectionPoint.xy = ScreenSpaceReflectionPoint.xy * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Compute the sreen space reflection vector as the difference of the two screen space points
	float3 ScreenSpaceReflectionVec = normalize(ScreenSpaceReflectionPoint.xyz - ScreenSpacePos.xyz);
	
	float3 OutReflectionColor;
	GetReflection(ScreenSpaceReflectionVec, ScreenSpacePos.xyz, OutReflectionColor);
	
	return float4(OutReflectionColor.xyz, 1.0f);
}

#endif //_SSR_HLSL