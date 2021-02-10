#ifndef _VS_SKYBOX_HLSL_
#define _VS_SKYBOX_HLSL_

#include "ALL_CommonBuffers.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 positionWS : TEXCOORD0;
};

cbuffer MatrixBuffer : register(b0)
{
	matrix g_worldMatrix;
	matrix g_viewMatrix;
	matrix g_projectionMatrix;
	matrix g_paddingMatrix;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	float4 worldPos = mul(float4(input.position.xyz, 1.0f), g_worldMatrix);
	output.positionWS = worldPos.xyz;

	output.position = mul(worldPos, g_viewMatrix);
	output.position = mul(output.position, g_projectionMatrix).xyww;

	return output;
}

#endif // _VS_SKYBOX_HLSL_
