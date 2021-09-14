#ifndef _PS_SIMPLECOLOR_HLSL_
#define _PS_SIMPLECOLOR_HLSL_

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


struct PS_OUTPUT
{
	float4 Color : SV_Target0;
	float4 Normal : SV_Target1;
};

PS_OUTPUT main(PixelInputType input) : SV_TARGET
{
	PS_OUTPUT output;
	output.Color = float4(0, 0, 0, 1);
	output.Normal = float4(0, 1, 0, 1);
	return output;
}

#endif // _PS_SIMPLECOLOR_HLSL_
