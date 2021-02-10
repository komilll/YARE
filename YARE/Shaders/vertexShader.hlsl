#ifndef _VERTEX_SHADER_HLSL
#define _VERTEX_SHADER_HLSL

#include "ALL_CommonBuffers.hlsl"

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD0;
	uint textureID : TEXCOORD1;
};

cbuffer SceneConstantBuffer : register(b0)
{
	matrix g_worldMatrix;
	matrix g_viewMatrix;
	matrix g_projMatrix;
	matrix g_paddingMatrix;
};

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	float4 position = float4(input.position, 1.0f);
	position = mul(position, g_worldMatrix);
	position = mul(position, g_viewMatrix);
	output.position = mul(position, g_projMatrix);
	
	//output.position = float4(input.position, 1.0f);
	
	output.normal = input.normal;
	output.uv = input.uv;
	output.binormal = input.binormal;
	output.tangent = input.tangent;
	
	output.textureID = input.textureID;
	
	return output;
}

#endif //_VERTEX_SHADER_HLSL