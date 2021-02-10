#ifndef _CONSTANSTS_HLSL
#define _CONSTANSTS_HLSL

const static float PI = 3.14159265358979323846;
const static float INV_PI = 0.31830988618379067154;
const static float INV_2PI = 0.15915494309189533577;
const static float INV_4PI = 0.07957747154594766788;

const static float3 textureIdColors[16] =
{
	float3(255.0f / 255.0f, 153.0f / 255.0f, 0.0f),					//1
	float3(153.0f / 255.0f, 204.0f / 255.0f, 255.0f / 255.0f),		//2
	float3(255.0f / 255.0f, 102.0f / 255.0f, 204.0f / 255.0f),		//3
	float3(0.0f / 255.0f, 255.0f / 255.0f, 204.0f / 255.0f),		//4
	float3(0.0f / 255.0f, 102.0f / 255.0f, 255.0f / 255.0f),		//5
	float3(255.0f / 255.0f, 80.0f / 255.0f, 80.0f / 255.0f),		//6
	float3(51.0f / 255.0f, 153.0f / 255.0f, 51.0f / 255.0f),		//7
	float3(153.0f / 255.0f, 204.0f / 255.0f, 255.0f / 255.0f),		//8
	float3(204.0f / 255.0f, 255.0f / 255.0f, 204.0f / 255.0f),		//9
	float3(102.0f / 255.0f, 102.0f / 255.0f, 255.0f / 255.0f),		//10
	float3(255.0f / 255.0f, 102.0f / 255.0f, 179.0f / 255.0f),		//11
	float3(51.0f / 153.0f, 102.0f / 255.0f, 255.0f / 255.0f),		//12
	float3(204.0f / 153.0f, 255.0f / 255.0f, 102.0f / 255.0f),		//13
	float3(0.0f / 153.0f, 153.0f / 255.0f, 51.0f / 255.0f),			//14
	float3(153.0f / 153.0f, 153.0f / 255.0f, 102.0f / 255.0f),		//15
	float3(0.0f / 153.0f, 102.0f / 255.0f, 204.0f / 255.0f)			//16
};

#endif //_CONSTANSTS_HLSL