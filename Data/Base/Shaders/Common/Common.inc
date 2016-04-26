#pragma once

#include "Platforms.h"

const static float PI = 3.1415926535897932f;

float4 uintToFloat4(uint x)
{
	float4 result;
	result.r = (x & 0xFF) / 255.0;
	result.g = ((x >> 8)  & 0xFF) / 255.0;
	result.b = ((x >> 16) & 0xFF) / 255.0;
	result.a = ((x >> 24) & 0xFF) / 255.0;
	
	return result;	
}
