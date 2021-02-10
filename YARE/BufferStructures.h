#pragma once

#ifndef _BUFFER_STRUCTURES_H_
#define _BUFFER_STRUCTURES_H_

#include "pch.h"
using namespace DirectX;

typedef struct _constantBufferStruct {
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;

	XMMATRIX paddingMatrix;
} ConstantBufferStruct;
static_assert((sizeof(ConstantBufferStruct) % 256) == 0, "Constant Buffer size must be 256-byte aligned");

typedef struct _sceneConstantBuffer
{
	XMMATRIX projectionToWorld;
	XMFLOAT4 cameraPosition;
	int frameCount;
	int specularIndex;
	int normalIndex;

	float padding[41] = {};
} SceneConstantBuffer;
static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "Scene Constant Buffer size must be 256-byte aligned");

typedef struct _aoConstantBuffer
{
	float aoRadius = 1.0f;
	float minT = 1e-4f;
	int accFrames;
	int sampleCount = 1;
	int maxFrames = 0;

	float padding[59] = {};
} AoConstantBuffer;
static_assert((sizeof(AoConstantBuffer) % 256) == 0, "Ao Constant Buffer size must be 256-byte aligned");

typedef struct _giConstantBuffer
{
	int useIndirect = 1;
	int accFrames = 0;
	float roughness = 0.75f;
	float metallic = 0.0f;

	int bounceCount = 1;
	int sqrtMaxFrames = 10;
	int maxFrames = 100;
	int samplingType = 1;

	bool useSkybox;
	XMFLOAT3 sunDirection = XMFLOAT3{ 0.419f, 0.318f, 0.061f };

	float padding[52] = {};
} GiConstantBuffer;
static_assert((sizeof(GiConstantBuffer) % 256) == 0, "GI Constant Buffer size must be 256-byte aligned");


typedef struct _cameraConstantBuffer
{
	XMFLOAT2 jitterCamera;
	float fNumber = 32.0f;
	float focalLength = 1.0f;

	XMFLOAT4 cameraU;
	XMFLOAT4 cameraV;
	XMFLOAT4 cameraW;

	float lensRadius;

	float padding[47] = {};
} CameraConstantBuffer;
static_assert((sizeof(CameraConstantBuffer) % 256) == 0, "Camera Constant Buffer size must be 256-byte aligned");

typedef struct _lightConstantBuffer
{
	XMFLOAT4 lightDiffuseColor[16];
	XMFLOAT4 lightPositionAndType[16];
} LightConstantBuffer;
static_assert((sizeof(LightConstantBuffer) % 256) == 0, "Light Constant Buffer size must be 256-byte aligned");

typedef struct _postprocessConstantBuffer
{
	float exposure = -9.8f;
	float padding[63];
} PostprocessConstantBuffer;
static_assert((sizeof(PostprocessConstantBuffer) % 256) == 0, "Postprocess Constant Buffer size must be 256-byte aligned");

#endif // !_BUFFER_STRUCTURES_H_
