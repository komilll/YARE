#ifndef _VS_POSTPROCESS_HLSL_
#define _VS_POSTPROCESS_HLSL_

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

PixelInputType main(VertexInputType input)
{
	PixelInputType output;
	
	output.position = float4(input.position, 1.0f);
	
	output.normal = input.normal;
	output.uv = input.uv;
	output.binormal = input.binormal;
	output.tangent = input.tangent;
	
	output.textureID = input.textureID;
	
	return output;
}

#endif //_VS_POSTPROCESS_HLSL_