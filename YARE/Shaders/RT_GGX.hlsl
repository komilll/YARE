#ifndef _RT_GGX_HLSL_
#define _RT_GGX_HLSL_

#include "RT_HelperFunctions.hlsl"
#include "UniformDistribution.hlsl"
#include "SamplingHelper.hlsl"
#include "ALL_CommonBuffers.hlsl"
#include "PS_BRDF_Helper.hlsl"

struct RayPayload
{
	float vis;
};

struct PayloadIndirect
{
	float3 color;
	uint rndSeed; // pixelIdx
	uint rndSeed2; // sampleSetIdx
	int pathLength;
	bool diffusePath;
	float roughness;
	float3 throughput;
};

struct LightConstantBuffer
{
	float4 lightDiffuseColor[16];
	float4 lightPositionAndType[16];
};

////

// Resources
ConstantBuffer<SceneConstantBuffer> g_sceneCB : register(b0);
ConstantBuffer<CameraConstantBuffer> g_cameraCB : register(b1);
ConstantBuffer<GiConstantBuffer> g_giCB : register(b2);
ConstantBuffer<LightConstantBuffer> g_lightCB : register(b3);

RWTexture2D<float4> RTOutput : register(u0);

RaytracingAccelerationStructure SceneBVH : register(t0);
ByteAddressBuffer indices : register(t1);
StructuredBuffer<Vertex> vertices : register(t2);
Texture2D<float4> NormalTextureInput : register(t3);
Texture2D<float4> albedoTexture : register(t4);
Texture2D<float4> specularTexture : register(t5);
Texture2D<float4> dfgTexture : register(t6);
TextureCube<float4> skyboxTexture : register(t7);

SamplerState g_sampler : register(s0);
////

static float2 SamplePoint(in uint pixelIdx, inout uint setIdx)
{
	const uint permutation = setIdx * (DispatchRaysDimensions().x * DispatchRaysDimensions().y) + pixelIdx;
	setIdx += 1;
	return SampleCMJ2D(g_giCB.accFrames, g_giCB.sqrtMaxFrames, g_giCB.sqrtMaxFrames, permutation);
}

void loadHitData(out float3 normal, out float3 tangent, out float3 bitangent, in BuiltInTriangleIntersectionAttributes attribs)
{
	uint indexSizeInBytes = 4;
	uint indicesPerTriangle = 3;
	uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
	uint baseIndex = PrimitiveIndex() * triangleIndexStride;
	
	const uint3 indices_ = indices.Load3(baseIndex);
	float3 vertexNormals[3] = { vertices[indices_[0]].normal, vertices[indices_[1]].normal, vertices[indices_[2]].normal };
	float3 vertexTangents[3] = { vertices[indices_[0]].tangent, vertices[indices_[1]].tangent, vertices[indices_[2]].tangent };
	float3 vertexBitangents[3] = { vertices[indices_[0]].binormal, vertices[indices_[1]].binormal, vertices[indices_[2]].binormal };
	
	normal = HitAttribute(vertexNormals, attribs);
	tangent = HitAttribute(vertexTangents, attribs);
	bitangent = HitAttribute(vertexBitangents, attribs);
}

float3 shootIndirectRay(float3 rayOrigin, float3 rayDir, float minT, PayloadIndirect payload)
{
	RayDesc rayColor = { rayOrigin, minT, rayDir, 1e+38f };
	TraceRay(SceneBVH, RAY_FLAG_FORCE_OPAQUE, 0xFF, 1, 2, 1, rayColor, payload);
	return payload.color;
}
	
float shadowRayVisibility(float3 origin, float3 dir, float tMin, float tMax)
{
	RayDesc visRay = { origin, tMin, dir, tMax };
	RayPayload payload;
	payload.vis = 0.0f;
	TraceRay(SceneBVH, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER, 0xFF, 0, 2, 0, visRay, payload);
	
	return payload.vis;
}

float3 Fresnel(in float3 specAlbedo, in float3 h, in float3 l)
{
	float3 fresnel = specAlbedo + (1.0f - specAlbedo) * pow((1.0f - saturate(dot(l, h))), 5.0f);

    // Fade out spec entirely when lower than 0.1% albedo
	fresnel *= saturate(dot(specAlbedo, 333.0f));

	return fresnel;
}

float3 SampleGGXVisibleNormal(float3 wo, float ax, float ay, float u1, float u2)
{
    // Stretch the view vector so we are sampling as though
    // roughness==1
	float3 v = normalize(float3(wo.x * ax, wo.y * ay, wo.z));

    // Build an orthonormal basis with v, t1, and t2
	float3 t1 = (v.z < 0.999f) ? normalize(cross(v, float3(0, 0, 1))) : float3(1, 0, 0);
	float3 t2 = cross(t1, v);

    // Choose a point on a disk with each half of the disk weighted
    // proportionally to its projection onto direction v
	float a = 1.0f / (1.0f + v.z);
	float r = sqrt(u1);
	float phi = (u2 < a) ? (u2 / a) * PI : PI + (u2 - a) / (1.0f - a) * PI;
	float p1 = r * cos(phi);
	float p2 = r * sin(phi) * ((u2 < a) ? 1.0f : v.z);

    // Calculate the normal in this stretched tangent space
	float3 n = p1 * t1 + p2 * t2 + sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2)) * v;

    // Unstretch and normalize the normal
	return normalize(float3(ax * n.x, ay * n.y, max(0.0f, n.z)));
}

float SmithGGXMasking(float3 n, float3 l, float3 v, float a2)
{
	float dotNL = saturate(dot(n, l));
	float dotNV = saturate(dot(n, v));
	float denomC = sqrt(a2 + (1.0f - a2) * dotNV * dotNV) + dotNV;

	return 2.0f * dotNV / denomC;
}

float SmithGGXMaskingShadowing(float3 n, float3 l, float3 v, float a2)
{
	float dotNL = saturate(dot(n, l));
	float dotNV = saturate(dot(n, v));

	float denomA = dotNV * sqrt(a2 + (1.0f - a2) * dotNL * dotNL);
	float denomB = dotNL * sqrt(a2 + (1.0f - a2) * dotNV * dotNV);

	return 2.0f * dotNL * dotNV / (denomA + denomB);
}

#define SAMPLE_UNIFORM 0
#define SAMPLE_MJ 1
#define SAMPLE_RANDOM 2

[shader("raygeneration")]
void RayGen()
{
	// Accumulate for limited amount of frames
	if (g_giCB.maxFrames > 0 && g_giCB.accFrames >= g_giCB.maxFrames)
	{
		return;
	}
	uint2 LaunchIndex = DispatchRaysIndex().xy;
	uint2 LaunchDimensions = DispatchRaysDimensions().xy;
	float4 normalAndDepth = NormalTextureInput.Load(int3(LaunchIndex, 0));

	// No geometry hit - skip pixel and use skybox data
	if (normalAndDepth.w == 0)
	{
		RTOutput[LaunchIndex] = albedoTexture.Load(int3(LaunchIndex, 0));
		return;
	}
	
	// Calculate seeds and offset
	uint seed = 0;
	uint seed2 = 0;
	float2 offset = float2(0, 0);
	if (g_giCB.samplingType == SAMPLE_UNIFORM)
	{
		seed = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_giCB.maxFrames, 16);
		seed2 = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_giCB.maxFrames, 17);
		offset = HammersleyDistribution(g_giCB.accFrames, g_giCB.maxFrames, uint2(seed, seed2));
	}
	else if (g_giCB.samplingType == SAMPLE_MJ)
	{
		const uint pixelIdx = LaunchIndex.y * LaunchDimensions.x + LaunchIndex.x;
		uint sampleSetIdx = 0;
		offset = SamplePoint(pixelIdx, sampleSetIdx);
		seed = pixelIdx;
		seed2 = sampleSetIdx;
	}
	else if (g_giCB.samplingType == SAMPLE_RANDOM)
	{
		seed = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_sceneCB.frameCount, 16);
		seed2 = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_sceneCB.frameCount, 17);
			
		offset.x = nextRand(seed);
		offset.y = nextRand(seed2);
	}
	
	// Calculate primary ray origin and direction
	float3 primaryRayOrigin = g_sceneCB.cameraPosition.xyz;
	float3 primaryRayDirection;
	GenerateCameraRay(LaunchIndex, LaunchDimensions, g_sceneCB.projectionToWorld, primaryRayOrigin, primaryRayDirection, offset);
	
	// Prepare payload
	PayloadIndirect indirectPayload;
	indirectPayload.color = float3(0, 0, 0);
	indirectPayload.roughness = 0;
	indirectPayload.rndSeed = seed;
	indirectPayload.rndSeed2 = seed2;
	indirectPayload.pathLength = 0;
	indirectPayload.diffusePath = false;
	indirectPayload.throughput = float3(1, 1, 1);
		
	// Calculate pixel color in current pass and merge with previous frames
	float4 finalColor = float4(shootIndirectRay(primaryRayOrigin, primaryRayDirection, 1e-3f, indirectPayload), 1.0f);
	bool colorsNan = any(isnan(finalColor));
	if (colorsNan)
	{
		finalColor = float4(0, 0, 0, 1);
	}
	
	// Clamp maximum brightness
	const float FP16Max = 65000.0f;
	finalColor = clamp(finalColor, 0.0f, FP16Max);
	
	// If not first frame - lerp values of current and previous frames
	if (g_giCB.accFrames > 0)
	{
		float4 prevScene = RTOutput[LaunchIndex];
		finalColor = ((float) g_giCB.accFrames * prevScene + finalColor) / ((float) g_giCB.accFrames + 1.0f);
	}
	RTOutput[LaunchIndex] = finalColor;
}

[shader("miss")]
void Miss(inout RayPayload payload : SV_RayPayload)
{
	payload.vis = 1.0f;
}

[shader("closesthit")]
void ClosestHit(inout PayloadIndirect payload, in BuiltInTriangleIntersectionAttributes attribs)
{

}

[shader("miss")]
void MissIndirect(inout PayloadIndirect payload : SV_RayPayload)
{
	// Use skybox as contribution if ray failed to hit geometry (right now, disabled for debug purposes)
	float3 rayDir = WorldRayDirection();
	rayDir.z = -rayDir.z;
	if (g_giCB.useSkybox)
	{
		payload.color += skyboxTexture.SampleLevel(g_sampler, rayDir, 0).rgb;
	}
}

//-------------------------------------------------------------------------------------------------
// Helper for computing the GGX visibility term
//-------------------------------------------------------------------------------------------------
float GGXV1(in float m2, in float nDotX)
{
	return 1.0f / (nDotX + sqrt(m2 + (1 - m2) * nDotX * nDotX));
}

//-------------------------------------------------------------------------------------------------
// Computes the GGX visibility term
//-------------------------------------------------------------------------------------------------
float GGXVisibility(in float m2, in float nDotL, in float nDotV)
{
	return GGXV1(m2, nDotL) * GGXV1(m2, nDotV);
}

float GGXSpecular(in float m, in float3 n, in float3 h, in float3 v, in float3 l)
{
	float NoH = saturate(dot(n, h));
	float NoL = saturate(dot(n, l));
	float NoV = saturate(dot(n, v));

	#if 0
	// Use UE4 Karis' roughness term a = roughness * roughness; a2 = a * a
	// And G term from his blog - http://graphicrants.blogspot.com/2013/08/specular-brdf-reference.html
	float d = Specular_D_GGX(m, NoH);
	float vis = Specular_G_GGX(m, NoV) * Specular_G_GGX(m, NoL);
	#else
	// Use original equations from MJP's Path Tracer
	// Results in slighlty different look
	float m2 = m * m;
		
	float x = NoH * NoH * (m2 - 1) + 1;
	float d = m2 / (PI * x * x);
	float vis = GGXVisibility(m2, NoL, NoV);
	#endif
	
	return d * vis;
}

float3 CalcLighting(in float3 normal, in float3 lightDir, in float3 peakIrradiance,
                    in float3 diffuseAlbedo, in float3 specularAlbedo, in float roughness,
                    in float3 positionWS, in float3 cameraPosWS, in float3 msEnergyCompensation)
{
	float3 lighting = diffuseAlbedo * INV_PI;

	float3 view = normalize(cameraPosWS - positionWS);
	const float NoL = saturate(dot(normal, lightDir));
	if (NoL > 0.0f)
	{
		const float NoV = saturate(dot(normal, view));
		float3 h = normalize(view + lightDir);

		float3 fresnel = Fresnel(specularAlbedo, h, lightDir);

		float specular = GGXSpecular(roughness, normal, h, view, lightDir);
		lighting += specular * fresnel * msEnergyCompensation;
	}

	return lighting * NoL * peakIrradiance;
}

float3 CalculateRadiance(inout PayloadIndirect payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	// Load hit data
	float3 hitPos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
	float3 triangleNormal, triangleTangent, triangleBitangent;
	loadHitData(triangleNormal, triangleTangent, triangleBitangent, attribs);

	// Load material data
	float4 albedo = albedoTexture.Load(int3(DispatchRaysIndex().xy, 0));
	float roughness = specularTexture.Load(int3(DispatchRaysIndex().xy, 0)).g;
	roughness = max(roughness, payload.roughness);
	float metallic = specularTexture.Load(int3(DispatchRaysIndex().xy, 0)).b;
	float3 normalTextureData = NormalTextureInput.Load(int3(DispatchRaysIndex().xy, 0));
	float3x3 tangentToWorld = float3x3(triangleTangent, triangleBitangent, triangleNormal);
	float3 normalWS = triangleNormal;
	
	// Use normal map
#if 0
	{
        // Sample the normal map, and convert the normal to world space
		//Texture2D normalMap = NormalTextureInput;

		uint indexSizeInBytes = 4;
		uint indicesPerTriangle = 3;
		uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
		uint baseIndex = PrimitiveIndex() * triangleIndexStride;
	
		const uint3 indices_ = indices.Load3(baseIndex);
	
		float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);
		float2 vertexUVs[3] =
		{
			vertices[indices_[0]].uv,
			vertices[indices_[1]].uv,
			vertices[indices_[2]].uv
		};
	
		float2 uv = barycentrics.x * vertexUVs[0] + barycentrics.y * vertexUVs[1] + barycentrics.z * vertexUVs[2];
		
		float3 normalTS;
		//normalTS.xy = normalMap.SampleLevel(g_sampler, uv, 0.0f).xy * 2.0f - 1.0f;
		normalTS.xy = normalTextureData.xy * 2.0f - 1.0f;
		normalTS.z = sqrt(1.0f - saturate(normalTS.x * normalTS.x + normalTS.y * normalTS.y));
		normalWS = normalize(mul(normalTS, tangentToWorld));

		tangentToWorld._31_32_33 = normalWS;
	}
#endif
	
	bool enableDiffuse = metallic < 1.0f;
	bool enableSpecular = !payload.diffusePath; /*&& payload.pathLength == 0;*/
	
	if (enableDiffuse == false && enableSpecular == false)
		return float3(0, 0, 0);
	
	const float3 diffuseAlbedo = lerp(albedo.rgb, 0.0f, metallic) * (enableDiffuse ? 1.0f : 0.0f);
	const float3 specularAlbedo = lerp(0.03f, albedo.rgb, metallic) * (enableSpecular ? 1.0f : 0.0f);

	float3 msEnergyCompensation = float3(1, 1, 1);
	float2 DFG = dfgTexture.SampleLevel(g_sampler, float2(saturate(dot(triangleNormal, -WorldRayDirection())), roughness), 0.0f).xy;
	float Ess = DFG.x;
	msEnergyCompensation = float3(1, 1, 1) + specularAlbedo * (1.0f / Ess - 1.0f);
	float3 worldOrigin = WorldRayOrigin();
	
	// Iterate over all lights
	float lightsCount = g_lightCB.lightPositionAndType[15].w;
	//for (int i = 0; i < lightsCount; i++)
	{
		// Calculate each light data
		//float3 lightColor = g_lightCB.lightDiffuseColor[i].rgb;
		//float3 toLight = g_lightCB.lightPositionAndType[i].xyz - hitPos;
		//float distToLight = length(toLight);
		//float3 L = toLight / distToLight;
		float3 V = g_sceneCB.cameraPosition.xyz;
		float3 N = triangleNormal.xyz;
		
		float3 L = normalize(g_giCB.sunDirection);
		float3 toLight = L;
		float distToLight = 1e+38f;
		
		// Check visibility
		float NoL = saturate(dot(N, L));
		float visibility = shadowRayVisibility(hitPos, L, 1e-3f, distToLight);
		//if (g_giCB.samplingType == SAMPLE_UNIFORM)
		//{
		//	NoL *= 2.0;
		//}
		
		// Calculate light contribution to point in world (diffuse lambertian term)
		payload.color += CalcLighting(triangleNormal, toLight, 1.0f, diffuseAlbedo, specularAlbedo,
					roughness, hitPos, worldOrigin, msEnergyCompensation) * visibility;
	}

	float2 brdfSample = float2(0, 0);
	float selector = 0;
	float3 rayDirWS = float3(0, 0, 0);

	if (g_giCB.useIndirect == 1)
	{
		uint2 LaunchIndex = DispatchRaysIndex().xy;
		uint2 LaunchDimensions = DispatchRaysDimensions().xy;
		float3 throughput = payload.throughput;
					
		if (g_giCB.samplingType == SAMPLE_UNIFORM)
		{
			if (payload.pathLength == 0)
			{
				brdfSample = HammersleyDistribution(g_giCB.accFrames, g_giCB.maxFrames, uint2(payload.rndSeed, payload.rndSeed2));
			}
			else
			{
				uint seed = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_sceneCB.frameCount, 16);
				uint seed2 = initRand(LaunchIndex.x + LaunchIndex.y * LaunchDimensions.x, g_sceneCB.frameCount, 17);
				brdfSample = HammersleyDistribution(payload.pathLength, g_giCB.bounceCount, uint2(seed, seed2));
			}
		}
		else if (g_giCB.samplingType == SAMPLE_MJ)
		{
			brdfSample = SamplePoint(payload.rndSeed, payload.rndSeed2);
		}
		else if (g_giCB.samplingType == SAMPLE_RANDOM)
		{
			brdfSample.x = nextRand(payload.rndSeed);
			brdfSample.y = nextRand(payload.rndSeed2);
		}
		
		selector = brdfSample.x;
		if (enableSpecular == false)
			selector = 0.0f;
		else if (enableDiffuse == false)
			selector = 1.0f;
		
		// Find next direction
		if (selector < 0.5f)
		{
			if (enableSpecular)
				brdfSample.x *= 2.0f;
			float3 rayDirTS = SampleDirectionCosineHemisphere(brdfSample.x, brdfSample.y);
			rayDirWS = normalize(mul(rayDirTS, tangentToWorld));
			throughput *= diffuseAlbedo;
		}
		else
		{
			if (enableDiffuse)
				brdfSample.x = (brdfSample.x - 0.5f) * 2.0f;

			float3 incomingRayDirWS = WorldRayDirection();
			float3 incomingRayDirTS = normalize(mul(incomingRayDirWS, transpose(tangentToWorld)));
					
#if 0
			//float3 rayDirTS = SampleDirectionCosineHemisphere(brdfSample.x, brdfSample.y);
					
			float a2 = roughness * roughness;
			float theta = acos(sqrt((1.0f - brdfSample.x) / ((a2 - 1.0f) * brdfSample.x + 1.0f)));
			float phi = 2.0f * PI * brdfSample.y;
					
			float3 normalTS = float3(0, 1, 0);
					
			float3 wo = -incomingRayDirTS;
			float3 wm = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
			float3 wi = 2.0f * dot(wo, wm) * wm - wo;
					
			float3 F = Specular_F_Schlick(specularAlbedo.rgb, saturate(dot(wi, wm)));
			float G = SmithGGXMaskingShadowing(wm, wi, wo, a2);

			rayDirWS = normalize(mul(wi, tangentToWorld));
			float weight = abs(dot(wo, wm) / (dot(triangleNormal, incomingRayDirWS) * dot(normalTS, wm)));

			if (dot(triangleNormal, rayDirWS) > 0.0f && dot(wi, wm) > 0.0f)
				throughput *= F * G * weight;
			else
				throughput *= 0;
#else
			float3 wo = incomingRayDirTS;
			float3 wm = SampleGGXVisibleNormal(-wo, roughness, roughness, brdfSample.x, brdfSample.y);
			float3 wi = reflect(wo, wm);

			float3 normalTS = float3(0.0f, 0.0f, 1.0f);

			float3 F = Fresnel(specularAlbedo.rgb, wm, wi);
			float G1 = SmithGGXMasking(normalTS, wi, -wo, roughness * roughness);
			float G2 = SmithGGXMaskingShadowing(normalTS, wi, -wo, roughness * roughness);

			throughput *= (F * (G2 / G1));
			float3 rayDirTS = wi;
			rayDirWS = normalize(mul(rayDirTS, tangentToWorld));
#endif
					
#if 1
			DFG = dfgTexture.SampleLevel(g_sampler, float2(saturate(dot(triangleNormal, -WorldRayDirection())), roughness), 0.0f).xy;
			Ess = DFG.x;
			throughput *= float3(1, 1, 1) + specularAlbedo * (1.0f / Ess - 1.0f);
#endif
		}
			
		//if (enableDiffuse && enableSpecular)
		//	throughput *= 2.0f;
			
		if (payload.pathLength < g_giCB.bounceCount)
		{
			// Prepare payload
			PayloadIndirect newPayload;
			newPayload.pathLength = payload.pathLength + 1;
			newPayload.rndSeed = payload.rndSeed;
			newPayload.rndSeed2 = payload.rndSeed2;
			newPayload.color = float3(0, 0, 0);
			newPayload.diffusePath = (selector < 0.5f);
			newPayload.roughness = roughness;
			newPayload.throughput = throughput;
			
			//if (newPayload.pathLength > 3)
			//{
			//	const float q = max(0.05f, 1.f - max(throughput.x, max(throughput.y, throughput.z)));
			//	uint r = initRand(newPayload.rndSeed, newPayload.rndSeed2 + ((uint) (selector * 100.0f)));
			//	if (nextRand(r) < q)
			//	{
			//		return payload.color;
			//	}
			//	throughput = throughput / (1.0f - q);
			//}
			
			// Calculate next ray bounce color contribution
			float3 bounceColor = shootIndirectRay(hitPos, rayDirWS, 1e-3f, newPayload);
			
			// Check to make sure our randomly selected, normal mapped diffuse ray didn't go below the surface.
			if (dot(normalWS, rayDirWS) <= 0.0f)
				bounceColor = float3(0, 0, 0);
			
			payload.color += bounceColor * throughput;
		}
		else
		{
			//float visibility = shadowRayVisibility(hitPos, rayDirWS, 1e-3f, 1e+38f);

			//float3 rayDir = rayDirWS;
			//rayDir.z = -rayDir.z;
			//if (g_giCB.useSkybox)
			//{
			//	float3 skyColor = skyboxTexture.SampleLevel(g_sampler, rayDir, 0).rgb;
			//	payload.color += visibility * skyColor * throughput;
			//}
		}
	}
	
	return payload.color;
}

[shader("closesthit")]
void ClosestHitIndirect(inout PayloadIndirect payload, in BuiltInTriangleIntersectionAttributes attribs)
{
	payload.color = CalculateRadiance(payload, attribs);
}

#endif //_RT_GGX_HLSL_