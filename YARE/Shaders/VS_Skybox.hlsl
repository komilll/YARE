#ifndef _VS_SKYBOX_HLSL_
#define _VS_SKYBOX_HLSL_

#include "ALL_CommonBuffers.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 positionWS : TEXCOORD0;
};

ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b0);

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	float4 worldPos = mul(float4(input.position.xyz, 1.0f), g_matricesCB.worldMatrix);
	output.positionWS = worldPos.xyz;

	output.position = mul(worldPos, g_matricesCB.viewMatrix);
	output.position = mul(output.position, g_matricesCB.projMatrix).xyww;

	return output;
}

#endif // _VS_SKYBOX_HLSL_
