#ifndef _PIXEL_SHADER_HLSL
#define _PIXEL_SHADER_HLSL

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

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
	//return float4(g_texture.Sample(g_sampler, input.uv).rgb, 1.0f);
	
	PS_OUTPUT output;
	output.Color = float4(g_texture.Sample(g_sampler, input.uv).rgb, 1.0f);
	output.Normal = float4(0, 0, 0, 0);
	return output;
}

#endif //_PIXEL_SHADER_HLSL