#pragma once

#include "Platforms.h"

// Common sampler states
SamplerState LinearSampler;
SamplerState LinearClampSampler;
SamplerState PointSampler;
SamplerState PointClampSampler;

const static float PI = 3.1415926535897932f;

float4 RGBA8ToFloat4(uint x)
{
  float4 result;
  result.r = x & 0xFF;
  result.g = (x >> 8)  & 0xFF;
  result.b = (x >> 16) & 0xFF;
  result.a = (x >> 24) & 0xFF;

  return result / 255.0;
}

// for when the input data is already float4
float4 RGBA8ToFloat4(float4 x)
{
  return x;
}

float3 RGB8ToFloat3(uint x)
{
  float3 result;
  result.r = x & 0xFF;
  result.g = (x >> 8)  & 0xFF;
  result.b = (x >> 16) & 0xFF;

  return result / 255.0;
}

float3 RGB10ToFloat3(uint x)
{
  float3 result;
  result.r = x & 0x3FF;
  result.g = (x >> 10) & 0x3FF;
  result.b = (x >> 20) & 0x3FF;

  return result / 1023.0;
}

float2 RG16FToFloat2(uint x)
{
  float2 result;
  result.r = f16tof32(x);
  result.g = f16tof32(x >> 16);

  return result;
}

float4 RGBA16FToFloat4(uint rg, uint ba)
{
  return float4(RG16FToFloat2(rg), RG16FToFloat2(ba));
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

float3 CubeMapDirection(float3 inDirection)
{
  return float3(inDirection.x, inDirection.z, -inDirection.y);
}

float InterleavedGradientNoise(float2 screenSpacePosition)
{
  float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
  return frac(magic.z * frac(dot(screenSpacePosition, magic.xy)));
}

float3 NormalizeAndGetLength(float3 v, out float len)
{
  float squaredLen = dot(v, v);
  float reciprocalLen = rsqrt(squaredLen);
  len = squaredLen * reciprocalLen;
  return v * reciprocalLen;
}

float Square(float x)
{
  return x * x;
}
float2 Square(float2 x)
{
  return x * x;
}
float3 Square(float3 x)
{
  return x * x;
}
float4 Square(float4 x)
{
  return x * x;
}

float AdjustContrast(float value, float contrast)
{
  float a = -contrast;
  float b = contrast + 1;
  return saturate(lerp(a, b, value));
}
