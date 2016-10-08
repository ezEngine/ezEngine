#pragma once

#include "Platforms.h"

const static float PI = 3.1415926535897932f;

float4 RGBA8ToFloat4(uint x)
{
	float4 result;
	result.r = (x & 0xFF) / 255.0;
	result.g = ((x >> 8)  & 0xFF) / 255.0;
	result.b = ((x >> 16) & 0xFF) / 255.0;
	result.a = ((x >> 24) & 0xFF) / 255.0;

	return result;
}

float3 RGB8ToFloat3(uint x)
{
	float3 result;
	result.r = (x & 0xFF) / 255.0;
	result.g = ((x >> 8)  & 0xFF) / 255.0;
	result.b = ((x >> 16) & 0xFF) / 255.0;

	return result;
}

float3 RGB10ToFloat3(uint x)
{
	float3 result;
	result.r = (x & 0x3FF) / 1023.0;
	result.g = ((x >> 10) & 0x3FF) / 1023.0;
	result.b = ((x >> 20) & 0x3FF) / 1023.0;

	return result;
}

float2 RG16FToFloat2(uint x)
{
	float2 result;
	result.r = f16tof32(x);
	result.g = f16tof32(x >> 16);

	return result;
}

float GetLuminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

float3 SrgbToLinear(float3 color)
{
	return (color < 0.04045) ? (color / 12.92) : pow(color / 1.055 + 0.0521327, 2.4);
}

float3 LinearToSrgb(float3 color)
{
	return (color < 0.0031308) ? (color * 12.92) : (1.055 * pow(color, 1.0 / 2.4) - 0.055);
}
