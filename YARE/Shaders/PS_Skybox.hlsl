#ifndef _PS_SKYBOX_HLSL_
#define _PS_SKYBOX_HLSL_

#include "ALL_CommonBuffers.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 positionWS : TEXCOORD0;
};

ConstantBuffer<SceneConstantBuffer> g_sceneCB : register(b0);
SamplerState baseSampler : register(s0);
TextureCube<float4> skyboxTexture : register(t0);

float4 main(PixelInputType input) : SV_TARGET
{
    //return float4(normalize(g_viewerPosition.xyz - input.positionWS) * float3(1, 1, -1), 1.0f);
	return skyboxTexture.Sample(baseSampler, normalize(input.positionWS - g_sceneCB.cameraPosition.xyz) * float3(1, 1, -1));
}

#endif // _PS_SKYBOX_HLSL_
