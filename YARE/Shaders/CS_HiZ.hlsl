#ifndef _CS_HIZ_HLSL_
#define _CS_HIZ_HLSL_

#include "ALL_CommonBuffers.hlsl"

Texture2D g_depthBuffer : register(t0);
SamplerState g_sampler : register(s0);

RWTexture2D<float2> hiZSrc : register(u0);
RWTexture2D<float2> hiZDst: register(u1);

ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b0);

#define max3(x, y, z)       ( max(max(x, y), z) )
#define min3(x, y, z)       ( min(min(x, y), z) )
#define min4(x, y, z, w)    ( min( min3(x, y, z), w) )
#define max4(x, y, z, w)    ( max( max3(x, y, z), w) )

float depthCStoVS(float depthCS)
{
	return g_matricesCB.projMatrix._43 / (g_matricesCB.projMatrix._34 * depthCS - g_matricesCB.projMatrix._33);
}

[numthreads(16, 16, 1)]
void generateHiZMip0(uint3 index : SV_DispatchThreadID)
{
	float depthCS = g_depthBuffer.Load(int3(index.xy, 0));
	float depthVS = depthCStoVS(depthCS);
	hiZSrc[index.xy] = float2(depthCS, depthCS);
}

[numthreads(16, 16, 1)]
void generateHiZMip(uint3 index : SV_DispatchThreadID)
{
	float2 a = hiZSrc.Load(int3(index.xy * 2 + int2(0, 0), 0));
	float2 b = hiZSrc.Load(int3(index.xy * 2 + int2(1, 0), 0));
	float2 c = hiZSrc.Load(int3(index.xy * 2 + int2(0, 1), 0));
	float2 d = hiZSrc.Load(int3(index.xy * 2 + int2(1, 1), 0));
		
	hiZDst[index.xy] = float2(min4(a.x, b.x, c.x, d.x), max4(a.y, b.y, c.y, d.y));
}

#endif //_CS_HIZ_HLSL_