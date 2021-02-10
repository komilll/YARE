#ifndef _UNIFORM_DISTRIBUTION_HLSL_
#define _UNIFORM_DISTRIBUTION_HLSL_

#include "Constants.hlsl"

float BitwiseInverseInRange(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // 1.0f / 2^32
}

float3 UniformSampleSphere(float u1, float u2)
{
	float z = 1.0 - 2.0 * u1; //u1 = [0, 1]; z = [-1, 1]
	float r = sqrt(1.0 - z * z);
	float phi = 2.0 * PI * u2; //u2 = [0, 1] - low-discrepancy
	float x = r * cos(phi);
	float y = r * sin(phi);
	return float3(x, y, z);
}

float3 UniformSampleHemisphere(float u1, float u2)
{
	const float r = sqrt(1.0f - u1 * u1);
	const float phi = 2 * PI * u2;
 
	return float3(cos(phi) * r, sin(phi) * r, u1);
}

float3 CosineSampleHemisphere(float u1, float u2)
{
	const float r = sqrt(u1);
	const float theta = 2 * PI * u2;
 
	const float x = r * cos(theta);
	const float y = r * sin(theta);
 
	return float3(x, y, sqrt(max(0.0f, 1 - u1)));
}

float2 HammersleyDistribution(uint i, uint N, uint2 random)
{
    // Transform to 0-1 interval (2.3283064365386963e-10 == 1.0 / 2^32)
#if 0
    float E1 = frac(float(i)/float(N) + float(random.x & 0xffff) / (1<<16));
    float E2 = float(reversebits(i) ^ random.y) * 2.3283064365386963e-10; // / 0x100000000
    return float2(E1, E2);
#else
	float2 u = float2(float(i) / float(N), reversebits(i) * 2.3283064365386963e-10);
	u = frac(u + float2(random & 0xffff) / (1 << 16));
	return u;
#endif
}

// [Tea, a Tiny Encryption Algorithm]
// [http://citeseer.ist.psu.edu/viewdoc/download?doi=10.1.1.45.281&rep=rep1&type=pdf]
// [http://gaim.umbc.edu/2010/07/01/gpu-random-numbers/]
uint2 randomTea(uint2 seed, uint iterations = 8)
{
	const uint k[4] = { 0xA341316C, 0xC8013EA4, 0xAD90777D, 0x7E95761E };
	const uint delta = 0x9E3779B9;
	uint sum = 0;
    [unroll]
	for (uint i = 0; i < iterations; ++i)
	{
		sum += delta;
		seed.x += (seed.x << 4) + k[0] ^ seed.x + sum ^ (seed.x >> 5) + k[1];
		seed.y += (seed.y << 4) + k[2] ^ seed.y + sum ^ (seed.y >> 5) + k[3];
	}
	return seed;
}

#endif // _ALL_UNIFORM_DISTRIBUTION_HLSL_
