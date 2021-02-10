#ifndef _PS_POSTPROCESS_HLSL_
#define _PS_POSTPROCESS_HLSL_

struct PostprocessConstantBuffer
{
	float exposure;
	float padding[63];
};

ConstantBuffer<PostprocessConstantBuffer> g_postprocessCB : register(b0);
Texture2D<float4> g_texture : register(t0);
SamplerState g_sampler : register(s0);

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 binormal : BINORMAL;
	float2 uv : TEXCOORD0;
	uint textureID : TEXCOORD1;
};

float3 ACESFilm(float3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float linearToSRGB(float r)
{
	if (r <= 0.0031308)
		return 12.92 * r;
	else
		return 1.055 * pow(r, 1.0 / 2.4) - 0.055;
}

float4 main(PixelInputType input) : SV_TARGET
{
	// Get backbuffer color
	float3 color = g_texture.Sample(g_sampler, input.uv).rgb;
	
	// Define scale
	const float FP16Scale = 0.0009765625f;
	
	// Apply exposure scale settings
	color *= exp2(g_postprocessCB.exposure) / FP16Scale;
	
	// Apply filmic curve and move from linear to sRGB space
	color = ACESFilm(color);
	color.r = linearToSRGB(color.r);
	color.g = linearToSRGB(color.g);
	color.b = linearToSRGB(color.b);
	
	return float4(color, 1.0f);
}

#endif //_PS_POSTPROCESS_HLSL_