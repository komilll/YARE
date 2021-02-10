#ifndef _ALL_UBER_BUFFER_HLSL_
#define _ALL_UBER_BUFFER_HLSL_

cbuffer UberBuffer : register(b7)
{
	float3 g_viewerPosition;
	float g_uberPad[61];
};

#endif //_ALL_UBER_BUFFER_HLSL_