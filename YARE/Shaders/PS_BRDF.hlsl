#ifndef _PS_BRDF_HLSL_
#define _PS_BRDF_HLSL_

#include <ALL_CommonBuffers.hlsl>
#include <PS_BRDF_Helper.hlsl>

struct PixelInputType
{
	float4 position : SV_POSITION;
	float3 positionWS : TEXCOORD0;
};

SamplerState BrdfLutSampleType : register(s1)
{
    Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    MipLODBias = 0.0f;
    MaxAnisotropy = 1;
    ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    BorderColor[0] = 0;
    BorderColor[1] = 0;
    BorderColor[2] = 0;
    BorderColor[3] = 0;
    MinLOD = 0;
    MaxLOD = D3D11_FLOAT32_MAX;
};

Texture2D albedoTexture 	: register(t0);
Texture2D roughnessTexture  : register(t1);
Texture2D normalTexture 	: register(t2);
Texture2D metallicTexture 	: register(t3);

float4 main(PixelInputType input) : SV_TARGET
{	
	//float roughness = g_hasRoughness == 0 ? g_roughnessValue : roughnessTexture.Sample(baseSampler, input.uv).r;
	//float metallic = g_hasMetallic == 0 ? g_metallicValue : metallicTexture.Sample(baseSampler, input.uv).r;

	//roughness = clamp(roughness, 0.05f, 0.999f);
	////roughness = pow(roughness, 2.0f);

	//input.normal = normalize(input.normal);
	//input.tangent = normalize(input.tangent);
	//input.binormal = normalize(input.binormal);

	//float3 N = 0;
	//if (g_hasNormal > 0){
	//	float3x3 TBN = transpose(float3x3(input.tangent, input.binormal, input.normal));
	//	N = normalTexture.Sample(baseSampler, input.uv).rgb * 2.0 - 1.0;
	//	N = normalize(mul(TBN, N));
	//}
	//else{
	//	N = normalize(input.normal);
	//}

	//const float3 V = normalize(input.viewDir.xyz);
	//const float3 L = normalize(input.pointToLight.xyz);
	//const float intensity = input.pointToLight.w;	
	//const float3 H = normalize(L + V);

	//const float NoV = abs(dot(N, V)) + 0.0001f; //avoid artifact - as in Frostbite
	//const float NoH = saturate(dot(N, H));
	//const float LoH = saturate(dot(L, H));
	//const float LoV = saturate(dot(L, V));
	//const float VoH = saturate(dot(V, H));
	//const float NoL = saturate(dot(L, N));

	//const float3 R = 2.0 * NoV * N - V;

	//float3 albedo = g_hasAlbedo == 0 ? float3(1.0f, 0.0f, 0.0f) : albedoTexture.Sample(baseSampler, input.uv);
	//albedo = float3(1.0f, 0.71f, 0.29f);
 //   albedo = saturate(albedo);
 //   //albedo = 1;
	//// albedo = albedo / (albedo + float3(1.0, 1.0, 1.0));
 //   //albedo = pow(albedo, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));

	////D component
	//float D = 1.0f;
	//if (g_ndfType == NDF_BECKMANN){
	//	D = Specular_D_Beckmann(roughness, NoH);
	//}
	//if (g_ndfType == NDF_GGX){
	//	D = Specular_D_GGX(roughness, NoH);
	//}
	//if (g_ndfType == NDF_BLINN_PHONG){
	//	D = Specular_D_Blinn_Phong(roughness, NoH);
	//}
	//D = saturate(D);

	////G component
	//float G = 1.0f;
	//float G1 = 1.0f; //Debug only purposes
	//float G2 = 1.0f; //Debug only purposes
	//if (g_geometryType == GEOM_IMPLICIT){
	//	G = G1 = Specular_G_Implicit(NoL, NoV);
	//}
	//if (g_geometryType == GEOM_NEUMANN){
	//	G = G1 = Specular_G_Neumann(NoL, NoV);
	//}
	//if (g_geometryType == GEOM_COOK_TORRANCE){
	//	G = G1 = Specular_G_CT(NoH, NoV, NoL, VoH);
	//}
	//if (g_geometryType == GEOM_KELEMEN){
	//	G = G1 = Specular_G_Kelemen(NoV, NoL, VoH);
	//}
	//if (g_geometryType == GEOM_BECKMANN){
	//	G1 = Specular_G_Beckmann(NoV, roughness);
	//	G2 = Specular_G_Beckmann(NoL, roughness);
	//	G = G1 * G2;
	//}
	//if (g_geometryType == GEOM_GGX){
	//	G1 = Specular_G_GGX(roughness, NoV);
	//	G2 = Specular_G_GGX(roughness, NoL);
	//	G = G1 * G2;
	//}
	//if (g_geometryType == GEOM_SCHLICK_BECKMANN){
	//	G1 = Specular_G_SchlickBeckmann(roughness, NoV);
	//	G2 = Specular_G_SchlickBeckmann(roughness, NoL);
	//	G = G1 * G2;
	//}
	//if (g_geometryType == GEOM_SCHLICK_GGX){
	//	G1 = Specular_G_SchlickGGX(roughness, NoV);
	//	G2 = Specular_G_SchlickGGX(roughness, NoL);
	//	G = G1 * G2;
	//}
	//G = saturate(G);

 //   const float3 diffuseColor = saturate(albedo - albedo * metallic);
 //   const float3 specularColor = saturate(lerp(0.04f, albedo, metallic));
	////F component
	//float3 F = 0;
	//if (g_fresnelType == FRESNEL_NONE){
 //       F = specularColor;
 //   } 
	//if (g_fresnelType == FRESNEL_SCHLICK){
 //       F = Specular_F_Schlick(specularColor, VoH);
 //   }
	//if (g_fresnelType == FRESNEL_CT){
 //       F = Specular_F_CT(specularColor, VoH);
 //   }
	//F = saturate(F);
	
	//const float3 prefilteredDiffuse = diffuseIBLTexture.Sample(baseSampler, N).rgb;
	//const float3 prefilteredSpecular = specularIBLTexture.SampleLevel(baseSampler, R, roughness * 5.0);

	//const float numeratorBRDF = D * F * G;
	//const float denominatorBRDF = max((4.0f * max(NoV, 0.0f) * max(NoL, 0.0f)), 0.001f);
	//const float BRDF = numeratorBRDF / denominatorBRDF;
 //   const float3 sunLight = numeratorBRDF * NoL * g_directionalLightColor.w;

 //   float2 envBRDF = enviroBRDF.Sample(BrdfLutSampleType, float2(NoV, roughness)).rg;
 //   float3 diffuse = prefilteredDiffuse * diffuseColor;
 //   float3 specular = prefilteredSpecular * (specularColor * envBRDF.x + envBRDF.y);
	//return float4(sunLight, 1.0f);
	////return MonteCarloSpecular(input.positionWS.xyz, diffuseColor, specularColor, N, V, roughness, 512, lightSettings[0]);
 //   uint lightCount;
 //   uint stride;
 //   lightSettings.GetDimensions(lightCount, stride);
    
 //   float4 color = 0;
 //   const float4 environmentLight = float4(diffuse + sunLight + specular, 1.0f);
 //   color = environmentLight;
    
 //   float3 diffAreaLight = DiffuseSphereLight_ViewFactor(input.positionWS.xyz, diffuse, N, lightSettings[0]);
 //   float3 specAreaLight = SpecularSphereLight_KarisMRP(input.positionWS.xyz, specular, roughness, N, V, lightSettings[0]);
    
 //   color = float4((diffAreaLight + specAreaLight) * lightSettings[0].color, 1.0f);
    
	//if (g_debugType == DEBUG_NONE){
 //       return color;
 //       return environmentLight;
 //   }
	//if (g_debugType == DEBUG_DIFF){
 //       return float4(/*diffuse + */ diffAreaLight * lightSettings[0].color, 1.0);
 //       return float4(diffuse, 1.0f);
 //   }
	//if (g_debugType == DEBUG_SPEC){
 //       return float4(/*specular + */ specAreaLight * lightSettings[0].color, 1.0);
 //       return float4(sunLight + specular, 1.0f);
	//}
	//if (g_debugType == DEBUG_ALBEDO){
	//	return float4(albedo, 1.0f);
	//}
	//if (g_debugType == DEBUG_NORMAL){
	//	return float4(N, 1.0f);
	//}
	//if (g_debugType == DEBUG_ROUGHNESS){
	//	return float4(roughness, roughness, roughness, 1.0f);
	//}
	//if (g_debugType == DEBUG_METALLIC){
	//	return float4(metallic, metallic, metallic, 1.0f);
	//}
	//if (g_debugType == DEBUG_NDF){
	//	return float4(D, D, D, 1.0f);
	//}
	//if (g_debugType == DEBUG_GEOM){
	//	return float4(G1, G2, 0.0f, 1.0f);
	//} 
	//if (g_debugType == DEBUG_FRESNEL){
	//	return float4(F, 1.0f);
	//}

	return float4(1.0f, 0.0f, 1.0f, 1.0f);
}

#endif //_PS_BRDF_HLSL_