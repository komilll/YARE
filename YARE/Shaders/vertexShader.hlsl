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
	//uint textureID : TEXCOORD1;
};

ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b0);

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	float4 position = float4(input.position, 1.0f);
	position = mul(position, g_matricesCB.worldMatrix);
	position = mul(position, g_matricesCB.viewMatrix);
	position = mul(position, g_matricesCB.projMatrix);
	output.position = position;
	
	//output.position = float4(input.position, 1.0f);
	
	output.normal = input.normal;
	output.uv = input.uv;
	output.binormal = input.binormal;
	output.tangent = input.tangent;
	
	//output.textureID = 0;//input.textureID;
	
	return output;
}

#endif //_VERTEX_SHADER_HLSL