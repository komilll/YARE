#ifndef _CS_HIZ_HLSL_
#define _CS_HIZ_HLSL_

Texture2D<float> depthBuffer : register(t0);

Texture2D<float2> hiZ		 : register(t0);
RWTexture2D<float2> hiZout   : register(u0);

//[numthreads(16, 16, 1)]
//void generateHiZMip0(uint3 index : SV_DispatchThreadID)
//{
//	int2 i = int2(index.xy) << ssrHiZMinMipLevel;
//	float depthCS = depthBuffer.Load(int3(i.xy, 0)).r;
//	float depthVS = cameraClipPlanes.y;
//    [flatten]
//	if (depthCS < 1.0 && all(index.xy < renderTargetSize))
//		depthVS = -depthCStoVS(depthCS);

//#ifdef CLIP_SPACE
//    float depth = depthCS;
//#else
//	float depth = depthVS;
//#endif

//	hiZout[index.xy] = float2(depth, depth);
//}

//[numthreads(16, 16, 1)]
//void generateHiZMip(uint3 index : SV_DispatchThreadID)
//{
//	float2 a = hiZ.Load(int3(index.xy * 2 + int2(0, 0), 0));
//	float2 b = hiZ.Load(int3(index.xy * 2 + int2(1, 0), 0));
//	float2 c = hiZ.Load(int3(index.xy * 2 + int2(0, 1), 0));
//	float2 d = hiZ.Load(int3(index.xy * 2 + int2(1, 1), 0));

//	hiZout[index.xy] = float2(min4(a.x, b.x, c.x, d.x), max4(a.y, b.y, c.y, d.y));
//}


#endif //_CS_HIZ_HLSL_