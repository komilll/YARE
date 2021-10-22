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

//cbuffer cbSSLR : register(b4)
//{
float2 cb_depthBufferSize = float2(1024.f, 512.f); // dimensions of the z-buffer
float cb_zThickness = 1.0f; // 0.15f - thickness to ascribe to each pixel in the depth buffer
//float cb_nearPlaneZ; // the camera's near z plane

float cb_stride = 1.0f; // 1.0f[?] - Step in horizontal or vertical pixels between samples. This is a float
// because integer math is slow on GPUs, but should be set to an integer >= 1.
float cb_maxSteps = 512.0f; // 512.0f - Maximum number of iterations. Higher gives better images but may be slow.
float cb_maxDistance = 100.0f; // 100.0f - Maximum camera-space distance to trace before returning a miss.
float cb_strideZCutoff = 10.0f; // More distant pixels are smaller in screen space. This value tells at what point to
// start relaxing the stride to give higher quality reflections for objects far from
// the camera.

float cb_numMips; // the number of mip levels in the convolved color buffer
float cb_fadeStart; // determines where to start screen edge fading of effect
float cb_fadeEnd; // determines where to end screen edge fading of effect
float cb_sslr_padding0; // padding for alignment
//};


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

float distanceSquared(float2 a, float2 b)
{
	a -= b;
	return dot(a, a);
}

bool intersectsDepthBuffer(float z, float minZ, float maxZ)
{
    /*
     * Based on how far away from the camera the depth is,
     * adding a bit of extra thickness can help improve some
     * artifacts. Driving this value up too high can cause
     * artifacts of its own.
     */
	float depthScale = min(1.0f, z * cb_strideZCutoff);
	z += cb_zThickness + lerp(0.0f, 2.0f, depthScale);
	return (maxZ >= z) && (minZ - cb_zThickness <= z);
}

void swap(inout float a, inout float b)
{
	float t = a;
	a = b;
	b = t;
}

float linearDepthTexelFetch(int2 hitPixel)
{
    // Load returns 0 for any value accessed out of bounds
	return hiZBuffer.Load(int3(hitPixel, 0)).r;
}

float3 positionWSFromDepthCS(float depthCS, float2 positionCS)
{
	float4 positionWS = mul(float4(positionCS, depthCS, 1.0f), g_matricesCB.invViewProjMatrix);
	return positionWS.xyz / positionWS.w;
}

// Returns true if the ray hit something
bool traceScreenSpaceRay(
    // Camera-space ray origin, which must be within the view volume
    float3 csOrig,
    // Unit length camera-space ray direction
    float3 csDir,
    // Number between 0 and 1 for how far to bump the ray in stride units
    // to conceal banding artifacts. Not needed if stride == 1.
    float jitter,
    // Pixel coordinates of the first intersection with the scene
    out float2 hitPixel,
    // Camera space location of the ray hit
    out float3 hitPoint)
{
	// TEMPORARY
	float cb_nearPlaneZ = zNear;
	
    // Clip to the near plane
	float rayLength = ((csOrig.z + csDir.z * cb_maxDistance) < cb_nearPlaneZ) ?
    (cb_nearPlaneZ - csOrig.z) / csDir.z : cb_maxDistance;
	float3 csEndPoint = csOrig + csDir * rayLength;

    // Project into homogeneous clip space
	float4 H0 = mul(float4(csOrig, 1.0f), g_matricesCB.projMatrix);
	H0.xy *= cb_depthBufferSize;
	float4 H1 = mul(float4(csEndPoint, 1.0f), g_matricesCB.projMatrix);
	H1.xy *= cb_depthBufferSize;
	float k0 = 1.0f / H0.w;
	float k1 = 1.0f / H1.w;

    // The interpolated homogeneous version of the camera-space points
	float3 Q0 = csOrig * k0;
	float3 Q1 = csEndPoint * k1;

    // Screen-space endpoints
	float2 P0 = H0.xy * k0;
	float2 P1 = H1.xy * k1;

    // If the line is degenerate, make it cover at least one pixel
    // to avoid handling zero-pixel extent as a special case later
	P1 += (distanceSquared(P0, P1) < 0.0001f) ? float2(0.01f, 0.01f) : 0.0f;
	float2 delta = P1 - P0;

    // Permute so that the primary iteration is in x to collapse
    // all quadrant-specific DDA cases later
	bool permute = false;
	if (abs(delta.x) < abs(delta.y))
	{
        // This is a more-vertical line
		permute = true;
		delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx;
	}

	float stepDir = sign(delta.x);
	float invdx = stepDir / delta.x;

    // Track the derivatives of Q and k
	float3 dQ = (Q1 - Q0) * invdx;
	float dk = (k1 - k0) * invdx;
	float2 dP = float2(stepDir, delta.y * invdx);

    // Scale derivatives by the desired pixel stride and then
    // offset the starting values by the jitter fraction
	float strideScale = 1.0f - min(1.0f, csOrig.z * cb_strideZCutoff);
	float stride = 1.0f + strideScale * cb_stride;
	dP *= stride;
	dQ *= stride;
	dk *= stride;

	P0 += dP * jitter;
	Q0 += dQ * jitter;
	k0 += dk * jitter;

    // Slide P from P0 to P1, (now-homogeneous) Q from Q0 to Q1, k from k0 to k1
	float4 PQk = float4(P0, Q0.z, k0);
	float4 dPQk = float4(dP, dQ.z, dk);
	float3 Q = Q0;

    // Adjust end condition for iteration direction
	float end = P1.x * stepDir;

	float stepCount = 0.0f;
	float prevZMaxEstimate = csOrig.z;
	float rayZMin = prevZMaxEstimate;
	float rayZMax = prevZMaxEstimate;
	float sceneZMax = rayZMax + 100.0f;
	for (;
        ((PQk.x * stepDir) <= end) && (stepCount < cb_maxSteps) &&
        !intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax) &&
        (sceneZMax != 0.0f);
        ++stepCount)
	{
		rayZMin = prevZMaxEstimate;
		rayZMax = (dPQk.z * 0.5f + PQk.z) / (dPQk.w * 0.5f + PQk.w);
		prevZMaxEstimate = rayZMax;
		if (rayZMin > rayZMax)
		{
			swap(rayZMin, rayZMax);
		}

		hitPixel = permute ? PQk.yx : PQk.xy;
        // You may need hitPixel.y = depthBufferSize.y - hitPixel.y; here if your vertical axis
        // is different than ours in screen space
		sceneZMax = linearDepthTexelFetch(int2(hitPixel));

		PQk += dPQk;
	}

    // Advance Q based on the number of steps
	Q.xy += dQ.xy * stepCount;
	hitPoint = Q * (1.0f / PQk.w);
	return intersectsDepthBuffer(sceneZMax, rayZMin, rayZMax);
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

#define MAX_REFLECTION_RAY_MARCH_STEP 0.01f
#define NUM_RAY_MARCH_SAMPLES 32

bool GetReflection(
	float3 ScreenSpaceReflectionVec,
	float3 ScreenSpacePos,
	out float3 ReflectionColor)
{
	// Raymarch in the direction of the ScreenSpaceReflectionVec until you get an intersection with your z buffer
	for (int RayStepIdx = 0; RayStepIdx < NUM_RAY_MARCH_SAMPLES; RayStepIdx++)
	{
		float3 RaySample = (RayStepIdx * MAX_REFLECTION_RAY_MARCH_STEP) * ScreenSpaceReflectionVec + ScreenSpacePos;
		float ZBufferVal = (hiZBuffer.Sample(pointSampler, RaySample.xy).r);
		float ZBufferOrigin = (hiZBuffer.Sample(pointSampler, ScreenSpacePos.xy).r);
				
		if (ZBufferOrigin > ZBufferVal)
		{
			ReflectionColor = colorBuffer.SampleLevel(pointSampler, RaySample.xy, 0).rgb;
			return true;
		}
	}

	return false;
}

inline void GenerateCameraRay(uint2 index, uint2 dimensions, float4x4 projectionToWorld, inout float3 origin, out float3 direction)
{
	float2 xy = index + float2(0.5f, 0.5f); // center in the middle of the pixel.
	float2 screenPos = (xy / (float2) dimensions) * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
	screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
	float4 world = mul(float4(screenPos, 0, 1), projectionToWorld);

	world.xyz /= world.w;
	direction = normalize(world.xyz - origin);
}

float4 SSR(PixelInputType input) : SV_TARGET
{
	float3 normalVS = normalBuffer.Sample(pointSampler, input.uv).xyz;
	if (!any(normalVS))
	{
		return float4(colorBuffer.SampleLevel(pointSampler, input.uv, 0).rgb, 1.0f);
	}
	normalVS = mul(float4(normalVS, 0.0f), g_matricesCB.viewMatrix).xyz;
	
	float depthCS = hiZBuffer.Sample(pointSampler, input.uv).r;
	//float depthCS = depthVStoCS(depth);
	float3 rayOriginWS = positionWSFromDepthCS(depthCS, input.uv);
	float3 rayOriginVS = mul(float4(rayOriginWS, 1.0f), g_matricesCB.viewMatrix).xyz;
	float3 rayOriginCS = mul(float4(rayOriginVS, 1.0f), g_matricesCB.projMatrix).xyz;
	
	// Calculate primary ray origin and direction
	float3 primaryRayOrigin = g_constantBuffer.cameraPosition.xyz;
	float3 primaryRayDirection;
	GenerateCameraRay(input.uv * float2(1024.0f, 512.0f), float2(1024.0f, 512.0f), g_matricesCB.invViewProjMatrix, primaryRayOrigin, primaryRayDirection);
	
	//float3 toPositionVS = normalize(mul(float4(primaryRayDirection, 1.0f), g_matricesCB.viewMatrix).xyz);
	float3 rayDirectionVS = reflect(primaryRayDirection, normalVS);
	float3 rayDirectionCS = mul(float4(rayOriginVS, 0.0f), g_matricesCB.projMatrix).xyz;
	rayDirectionCS = normalize(rayDirectionCS);
	
	//rayDirectionCS = float3(0, -1, 0);
	//rayDirectionVS.x = rayDirectionVS.x * 2.0f - 1.0f; // Transform from [0, 1] to [-1, 1]
	
	float jitter = 0.0f;
	// out parameters
	float2 hitPixel = float2(0.0f, 0.0f);
	float3 hitPoint = float3(0.0f, 0.0f, 0.0f);
	
	//bool intersection = traceScreenSpaceRay(rayOriginVS, rayDirectionVS, jitter, hitPixel, hitPoint);
	//return float4(primaryRayDirection, 1.0f);
	
	float3 ReflectionColor = float3(0, 0, 0);
	if (GetReflection(primaryRayDirection, float3(input.uv, depthCS), ReflectionColor))
	{
		return float4(ReflectionColor + colorBuffer.SampleLevel(pointSampler, input.uv, 0).rgb, 1.0f);
	}
	
	return float4(colorBuffer.SampleLevel(pointSampler, input.uv, 0).rgb, 1.0f);
	
	//return float4(rayDirectionVS, 1.0f);
	//return float4(linearize(depth), 0, 0, 1);
}

#endif //_SSR_HLSL