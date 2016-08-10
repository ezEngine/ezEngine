#pragma once

#include "Platforms.h"

const static float PI = 3.1415926535897932f;

float4 UIntToFloat4(uint x)
{
	float4 result;
	result.r = (x & 0xFF) / 255.0;
	result.g = ((x >> 8)  & 0xFF) / 255.0;
	result.b = ((x >> 16) & 0xFF) / 255.0;
	result.a = ((x >> 24) & 0xFF) / 255.0;
	
	return result;	
}

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 SrgbToLinear(float3 color)
{
	return (color < 0.04045) ? (color / 12.92) : pow((color + 0.055) / 1.055, 2.4);
}

float3 LinearToSrgb(float3 color)
{
	return (color < 0.0031308) ? (color * 12.92) : (1.055 * pow(color, 1.0 / 2.4) - 0.055);
}
