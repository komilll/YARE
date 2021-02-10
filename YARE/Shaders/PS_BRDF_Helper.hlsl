#ifndef _PS_BRDF_HELPER_HLSL_
#define _PS_BRDF_HELPER_HLSL_

#include "Constants.hlsl"

//Normal distribution functions
float Specular_D_Beckmann(float roughness, float NoH)
{
    const float a = roughness * roughness;
	const float a2 = a * a;
    const float NoH2 = NoH * NoH;
    return exp( (NoH2-1.0f) / (a2*NoH2) ) / (a2*PI*NoH2*NoH2);
}

float Specular_D_GGX(float roughness, float NoH)
{
    const float a = roughness * roughness;
	const float a2 = a * a;
    const float NoH2 = NoH * NoH;
    return a2 / (PI * pow((NoH2*(a2-1)+1), 2) );
}

float Specular_D_Blinn_Phong(float roughness, float NoH)
{
	const float a = roughness * roughness;
    const float a2 = a * a;
    const float NoH2 = NoH * NoH;
    return pow(NoH, (2/a2-2)) / (PI*a2);
}

//Geometry shadowing functions
float Specular_G_Implicit(float NoL, float NoV)
{
	return NoL * NoV;
}

float Specular_G_Neumann(float NoL, float NoV)
{
	return (NoL * NoV) / max(NoL, NoV);
}

float Specular_G_CT(float NoH, float NoV, float NoL, float VoH)
{
	return min( min(1.0f, (2.0f*NoH*NoV/VoH)), (2.0f*NoH*NoL/VoH) );
}

float Specular_G_Kelemen(float NoV, float NoL, float VoH)
{
	return NoL*NoV/(VoH*VoH);
}

float Specular_G_Beckmann(float NoV, float roughness)
{
	const float a = roughness * roughness;
	const float c = NoV / (a * sqrt(1 - NoV*NoV));
	if (c >= 1.6f)
		return 1.0f;
	else
		return (3.535f*c + 2.181f*c*c) / (1.0f + 2.276f*c + 2.577f*c*c);
}

float Specular_G_GGX(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float NoV2 = NoV * NoV;
	return (2.0f*NoV) / (NoV + sqrt(a2+(1-a2)*NoV2));
}

float Specular_G_SchlickBeckmann(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float k = a * sqrt(2.0f/PI);
	return NoV / (NoV*(1-k) + k);
}

float Specular_G_SchlickGGX(float roughness, float NoV)
{
	const float a = roughness * roughness;
	const float k = a/2.0f;
	return NoV / (NoV*(1.0f-k) + k);
}

//Fresnel functions
float3 Specular_F_Schlick(float3 f0, float VoH)
{
	return f0 + (1.0-f0)*pow(1.0-VoH, 5.0);
}
float3 Specular_F_Schlick(float3 f0, float3 f90, float VoH)
{
	return f0 + (f90-f0)*pow(1.0-VoH, 5.0);
}

float3 Specular_F_CT(float3 f0, float VoH)
{
    const float3 f0Sqrt = min(0.999, sqrt(f0));
	const float3 eta = (1.0 + f0Sqrt) / (1.0 - f0Sqrt);
	const float3 g = sqrt(eta*eta + VoH*VoH - 1.0);
	const float3 c = VoH;
    
    float3 g_minus = g - VoH;
    float3 g_plus = g + VoH;
    float3 A = (g_minus / g_plus);
    float3 B = (g_plus * VoH - 1.0) / (g_minus * VoH + 1.0);

    return 0.5 * A * A * (1.0 + B * B);
}

//Disney diffuse function - modified by Frostbite
//https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf#page=11&zoom=auto,-265,590
//float Diffuse_Disney(float NoV, float NoL, float LoH, float roughness)
//{
//	float  energyBias     = lerp(0, 0.5,  roughness);
//	float  energyFactor   = lerp (1.0, 1.0 / 1.51,  roughness);
//	float3  fd90          = energyBias + 2.0 * LoH*LoH * roughness;
//    float3 f0             = 1.0f;
//    float lightScatter    = Specular_F_Schlick(f0, fd90, NoL);
//    float viewScatter     = Specular_F_Schlick(f0, fd90, NoV);
//	return  lightScatter * viewScatter * energyFactor;

//	// float fd90 = 0.5f + 2 * roughness * LoH * LoH;
//	// return (1.0f+(fd90-1.0f)*pow(1.0f-NoL, 5)) * (1.0f+(fd90-1.0f)*pow(1.0f-NoV, 5));
//}

#endif //_PS_BRDF_HELPER_HLSL_