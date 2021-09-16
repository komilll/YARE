#ifndef _SSR_HLSL
#define _SSR_HLSL

#include "ALL_CommonBuffers.hlsl"

SamplerState pointSampler : register(s0);

Texture2D hiZBuffer : register(t0);
Texture2D visibilityBuffer : register(t1);
Texture2D normalBuffer : register(t2);

RWTexture2D<float> visibilityOut : register(u0);

ConstantBuffer<MipConstantBuffer> g_mipCB : register(b0);
ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b1);
cbuffer g_preIntegrateCB : register(b2)
{
	float zNear;
	float zFar;
	float preIntegratePadding[62];
};

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

float3 hiZTrace(float3 p, float3 v)
{
	return float3(1, 1, 1);
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

float4 SSR(PixelInputType input) : SV_TARGET
{
	//return float4(input.uv, 0.0f, 1.0f);
	//return float4(1, 0, 0, 1);
	int3 loadIndices = int3(input.uv, 0);
	float depth = depthVStoCS(hiZBuffer.Sample(pointSampler, input.uv).r);
	float3 positionSS = float3(input.uv, depth);
	
	float3 viewRay = normalize( mul( float4(positionSS, 1.0f), g_matricesCB.invProjMatrix ) );
	float3 positionVS = viewRay * linearize(depth);
	
	float3 toPositionVS = normalize(positionVS);
	
	float3 normalVS = normalBuffer.Load(loadIndices).rgb;
	return float4(depth, depth, depth, 1.0f);
	
	if (dot(normalVS, float3(1.0f, 1.0f, 1.0f)) == 0.0f)
	{
		//return float4(0.0f, 0.0f, 0.0f, 0.0f);
	}
	
	float3 reflectVS = reflect(toPositionVS, normalVS);
	float4 positionPrimeSS4 = mul(float4(positionVS + reflectVS, 1.0f), g_matricesCB.projMatrix);
	float3 positionPrimeSS = (positionPrimeSS4.xyz / positionPrimeSS4.w);
	positionPrimeSS.x = positionPrimeSS.x * 0.5f + 0.5f;
	positionPrimeSS.y = positionPrimeSS.y * -0.5f + 0.5f;
	
	float3 reflectSS = positionPrimeSS - positionSS;
	
	//return float4(reflectSS.xyz, 1.0f);
}

#endif //_SSR_HLSL