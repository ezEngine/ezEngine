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
  result.g = (x >> 8) & 0xFF;
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
  result.g = (x >> 8) & 0xFF;
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
  return select(color < 0.04045, (color / 12.92), pow(color / 1.055 + 0.0521327, 2.4));
}

float3 LinearToSrgb(float3 color)
{
  return select(color < 0.0031308, (color * 12.92), (1.055 * pow(color, 1.0 / 2.4) - 0.055));
}

float3 CubeMapDirection(float3 inDirection)
{
  return float3(inDirection.x, inDirection.z, -inDirection.y);
}

float3 DecodeNormalTexture(float2 normalTex)
{
  float2 xy = normalTex.xy * 2.0f - 1.0f;
  float z = sqrt(max(1.0f - dot(xy, xy), 0.0));
  return float3(xy, z);
}

float3 DecodeNormalTexture(float4 normalTex)
{
  return DecodeNormalTexture(normalTex.xy);
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

float3 Colorize(float3 baseColor, float3 color, float mask)
{
  return baseColor * lerp(1, 2 * color, mask);
}

// https://iquilezles.org/articles/smin/
float SmoothMin(float a, float b, float k = 0.1)
{
  float h = max(k - abs(a - b), 0.0) / k;
  return min(a, b) - h * h * k * (1.0 / 4.0);
}

float SmoothMinCubic(float a, float b, float k = 0.1)
{
  float h = max(k - abs(a - b), 0.0) / k;
  return min(a, b) - h * h * h * k * (1.0 / 6.0);
}

// Returns 1 if pixel (cell) is inside a lit part of 'digit'. cell is in [0..2]x[0..4].
//
// Each glyph is 3x5 pixels. Every row is padded to 4 bits so that it occupies exactly
// one hex digit, which makes the constants below readable as the glyphs they encode:
// the 5 hex digits are the rows from top to bottom, and within a row the 3 low bits
// are the pixels from left to right (e.g. 7 = '###', 5 = '# #', 1 = '  #').
float SampleDigit(uint digit, int2 cell)
{
  static const uint s_Digits[10] =
  {
    0x75557, // 0
    0x26227, // 1
    0x71747, // 2
    0x71717, // 3
    0x55711, // 4
    0x74717, // 5
    0x74757, // 6
    0x71111, // 7
    0x75757, // 8
    0x75717  // 9
  };

  if (any(cell < 0) || cell.x > 2 || cell.y > 4 || digit > 9)
    return 0;

  return (s_Digits[digit] >> ((4 - cell.y) * 4 + (2 - cell.x))) & 1;
}

// Renders up to 3 decimal digits of 'x' into the cell grid, magnified by 'pixelSize'.
float SampleNumber(uint x, int2 cell, int pixelSize = 3)
{
  x = min(x, 999); // clamp to 3 digits

  cell /= pixelSize; // scale down to glyph coordinates
  if (cell.x < 0)
    return 0;

  uint slot = uint(cell.x) >> 2;       // which digit this pixel falls into
  int2 local = int2(cell.x & 3, cell.y); // position within that digit's slot

  uint numDigits = (x >= 100) ? 3 : ((x >= 10) ? 2 : 1);
  if (slot >= numDigits)
    return 0;

  // Shift the wanted digit into the ones place, most significant digit first.
  uint digit = x;
  uint exponent = numDigits - 1 - slot;
  if (exponent >= 1)
    digit /= 10;
  if (exponent >= 2)
    digit /= 10;

  return SampleDigit(digit % 10, local);
}
