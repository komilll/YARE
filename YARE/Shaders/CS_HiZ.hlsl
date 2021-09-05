#ifndef _CS_HIZ_HLSL_
#define _CS_HIZ_HLSL_

#include "ALL_CommonBuffers.hlsl"

Texture2D g_depthBuffer : register(t0);
SamplerState g_sampler : register(s0);

RWTexture2D<float> hiZMip0 : register(u0);
RWTexture2D<float> hiZMip1 : register(u1);
RWTexture2D<float> hiZMip2 : register(u2);

ConstantBuffer<MatricesConstantBuffer> g_matricesCB : register(b0);
cbuffer hiZBuffer : register(b1)
{
	int g_currentMip;
	int g_paddingHiZBuffer[63];
};

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
	hiZMip0[index.xy] = depthCStoVS(depthCS);
}

[numthreads(16, 16, 1)]
void generateHiZMip1(uint3 index : SV_DispatchThreadID)
{
	float a = hiZMip0.Load(int3(index.xy * 2 + int2(0, 0), 0));
	float b = hiZMip0.Load(int3(index.xy * 2 + int2(1, 0), 0));
	float c = hiZMip0.Load(int3(index.xy * 2 + int2(0, 1), 0));
	float d = hiZMip0.Load(int3(index.xy * 2 + int2(1, 1), 0));
		
	hiZMip1[index.xy] = min4(a.x, b.x, c.x, d.x);
}

[numthreads(16, 16, 1)]
void generateHiZMip2(uint3 index : SV_DispatchThreadID)
{
	float a = hiZMip1.Load(int3(index.xy * 2 + int2(0, 0), 0));
	float b = hiZMip1.Load(int3(index.xy * 2 + int2(1, 0), 0));
	float c = hiZMip1.Load(int3(index.xy * 2 + int2(0, 1), 0));
	float d = hiZMip1.Load(int3(index.xy * 2 + int2(1, 1), 0));
		
	hiZMip2[index.xy] = min4(a.x, b.x, c.x, d.x);
}


#endif //_CS_HIZ_HLSL_