#ifndef _SAMPLING_HELPER_HLSL_
#define _SAMPLING_HELPER_HLSL_

#include "Constants.hlsl"

// NVIDIA function - https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
uint initRand(uint val0, uint val1, uint backoff = 16)
{
	uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
	for (uint n = 0; n < backoff; n++)
	{
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}

// NVIDIA function - https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
float nextRand(inout uint s)
{
	s = (1664525u * s + 1013904223u);
	return float(s & 0x00FFFFFF) / float(0x01000000);
}

// NVIDIA function - https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
float3 getPerpendicularVector(float3 u)
{
	float3 a = abs(u);
	uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
	uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
	uint zm = 1 ^ (xm | ym);
	return cross(u, float3(xm, ym, zm));
}

// NVIDIA function - https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float3 bitangent = getPerpendicularVector(hitNorm);
	float3 tangent = cross(bitangent, hitNorm);
	float r = sqrt(randVal.x);
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// Modification of above NVIDIA function - https://github.com/NVIDIAGameWorks/GettingStartedWithRTXRayTracing/blob/master/11-OneShadowRayPerPixel/Data/Tutorial11/diffusePlus1ShadowUtils.hlsli
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm, float3 tangent, float3 bitangent)
{
	// Get 2 random numbers to select our sample with
	float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
	float r = sqrt(randVal.x);
	float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
	return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

//////
// Multi-jittered sampling [Kensler 2013] code
//////


uint CMJPermute(uint i, uint l, uint p)
{
	uint w = l - 1;
	w |= w >> 1;
	w |= w >> 2;
	w |= w >> 4;
	w |= w >> 8;
	w |= w >> 16;
	do
	{
		i ^= p;
		i *= 0xe170893d;
		i ^= p >> 16;
		i ^= (i & w) >> 4;
		i ^= p >> 8;
		i *= 0x0929eb3f;
		i ^= p >> 23;
		i ^= (i & w) >> 1;
		i *= 1 | p >> 27;
		i *= 0x6935fa69;
		i ^= (i & w) >> 11;
		i *= 0x74dcb303;
		i ^= (i & w) >> 2;
		i *= 0x9e501cc3;
		i ^= (i & w) >> 2;
		i *= 0xc860a3df;
		i &= w;
		i ^= i >> 5;
	}
    while (i >= l);
	return (i + p) % l;
}

float CMJRandFloat(uint i, uint p)
{
	i ^= p;
	i ^= i >> 17;
	i ^= i >> 10;
	i *= 0xb36534e5;
	i ^= i >> 12;
	i ^= i >> 21;
	i *= 0x93fc4795;
	i ^= 0xdf6e307f;
	i ^= i >> 17;
	i *= 1 | p >> 18;
	return i * (1.0f / 4294967808.0f);
}

 // Returns a 2D sample from a particular pattern using correlated multi-jittered sampling [Kensler 2013]
float2 SampleCMJ2D(uint sampleIdx, uint numSamplesX, uint numSamplesY, uint pattern)
{
	uint N = numSamplesX * numSamplesY;
	sampleIdx = CMJPermute(sampleIdx, N, pattern * 0x51633e2d);
	uint sx = CMJPermute(sampleIdx % numSamplesX, numSamplesX, pattern * 0x68bc21eb);
	uint sy = CMJPermute(sampleIdx / numSamplesX, numSamplesY, pattern * 0x02e5be93);
	float jx = CMJRandFloat(sampleIdx, pattern * 0x967a889b);
	float jy = CMJRandFloat(sampleIdx, pattern * 0x368cc8b7);
	return float2((sx + (sy + jx) / numSamplesY) / numSamplesX, (sampleIdx + jy) / N);
}

// Maps a value inside the square [0,1]x[0,1] to a value in a disk of radius 1 using concentric squares.
// This mapping preserves area, bi continuity, and minimizes deformation.
// Based off the algorithm "A Low Distortion Map Between Disk and Square" by Peter Shirley and
// Kenneth Chiu.
float2 SquareToConcentricDiskMapping(float x, float y)
{
	float phi = 0.0f;
	float r = 0.0f;

	float a = 2.0f * x - 1.0f;
	float b = 2.0f * y - 1.0f;

	if (a > -b)                      // region 1 or 2
	{
		if (a > b)                   // region 1, also |a| > |b|
		{
			r = a;
			phi = (PI / 4.0f) * (b / a);
		}
		else // region 2, also |b| > |a|
		{
			r = b;
			phi = (PI / 4.0f) * (2.0f - (a / b));
		}
	}
	else // region 3 or 4
	{
		if (a < b)                   // region 3, also |a| >= |b|, a != 0
		{
			r = -a;
			phi = (PI / 4.0f) * (4.0f + (b / a));
		}
		else // region 4, |b| >= |a|, but a==0 and b==0 could occur.
		{
			r = -b;
			if (b != 0)
				phi = (PI / 4.0f) * (6.0f - (a / b));
			else
				phi = 0;
		}
	}

	float2 result;
	result.x = r * cos(phi);
	result.y = r * sin(phi);
	return result;
}

// Returns a random cosine-weighted direction on the hemisphere around z = 1
float3 SampleDirectionCosineHemisphere(float u1, float u2)
{
	float2 uv = SquareToConcentricDiskMapping(u1, u2);
	float u = uv.x;
	float v = uv.y;

    // Project samples on the disk to the hemisphere to get a
    // cosine weighted distribution
	float3 dir;
	float r = u * u + v * v;
	dir.x = u;
	dir.y = v;
	dir.z = sqrt(max(0.0f, 1.0f - r));

	return dir;
}

#endif // _SAMPLING_HELPER_HLSL_
