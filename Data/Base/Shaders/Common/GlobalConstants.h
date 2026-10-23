#pragma once

#include "ConstantBufferMacros.h"
#include "Platforms.h"

#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS 0.002025

#define WIREFRAME_RENDER_PASS_COLOR 1
#define WIREFRAME_RENDER_PASS_MONOCHROME 2

#define EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY 1
#define EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY 2
#define EDITOR_RENDER_PASS_LIGHT_COUNT 3
#define EDITOR_RENDER_PASS_DECAL_COUNT 4
#define EDITOR_RENDER_PASS_TEXCOORDS_UV0 5
#define EDITOR_RENDER_PASS_TEXCOORDS_UV1 6
#define EDITOR_RENDER_PASS_VERTEX_COLORS0 7
#define EDITOR_RENDER_PASS_VERTEX_COLORS1 8
#define EDITOR_RENDER_PASS_VERTEX_NORMALS 9
#define EDITOR_RENDER_PASS_VERTEX_TANGENTS 10
#define EDITOR_RENDER_PASS_PIXEL_NORMALS 11
#define EDITOR_RENDER_PASS_DIFFUSE_COLOR 12
#define EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE 13
#define EDITOR_RENDER_PASS_SPECULAR_COLOR 14
#define EDITOR_RENDER_PASS_EMISSIVE_COLOR 15
#define EDITOR_RENDER_PASS_ROUGHNESS 16
#define EDITOR_RENDER_PASS_OCCLUSION 17
#define EDITOR_RENDER_PASS_DEPTH 18
#define EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC 19
#define EDITOR_RENDER_PASS_BONE_WEIGHTS 20

// GlobalTime and WorldTime wrap around at this value, see ezRenderContext::SetGlobalAndWorldTimeConstants.
// Must match ezShaderUtils::TimeWrapAround on the CPU side.
#define TIME_WRAP_AROUND 1000.0

CONSTANT_BUFFER(ezGlobalConstants, 0)
{
  // Use functions from CameraConstantsAccess.h to access these and derived camera properties.
  MAT4(CameraToScreenMatrix)
  [2];
  MAT4(ScreenToCameraMatrix)
  [2];
  MAT4(WorldToCameraMatrix)
  [2];
  MAT4(CameraToWorldMatrix)
  [2];
  MAT4(WorldToScreenMatrix)
  [2];
  MAT4(ScreenToWorldMatrix)
  [2];

  FLOAT4(ViewportSize); // x = width, y = height, z = 1 / width, w = 1 / height
  FLOAT4(ClipPlanes);   // x = near, y = far, z = 1 / far
  FLOAT1(MaxZValue);    // any screenspace z values smaller than this value are clamped. Used for directional shadows.

  FLOAT1(DeltaTime);
  FLOAT1(GlobalTime);   // wraps around at TIME_WRAP_AROUND to prevent precision issues with floating points, see WrapSafeRate() in this file
  FLOAT1(WorldTime);    // wraps around at TIME_WRAP_AROUND to prevent precision issues with floating points, see WrapSafeRate() in this file

  FLOAT1(Exposure);

  INT1(RenderPass);
  BOOL1(IsShadowPass);
  UINT1(NumMsaaSamples);
  BOOL1(IsOrthographicCamera);
};

#if EZ_ENABLED(PLATFORM_SHADER)

#  ifndef PI
#    define PI 3.1415926535897932
#  endif

// Rounds a rate (units per second) so that rate * TIME_WRAP_AROUND is a whole multiple of period.
// Any periodic function f(GlobalTime * rate) with the given period then does not jump when the time wraps around.
// E.g. use a period of 1 for texture scrolling in UV space with a repeating sampler, or 2 * PI for sin / cos (see WrapSafeAngularFrequency).
float WrapSafeRate(float rate, float period)
{
  const float step = period / TIME_WRAP_AROUND;
  return round(rate / step) * step;
}

float2 WrapSafeRate(float2 rate, float period)
{
  const float step = period / TIME_WRAP_AROUND;
  return round(rate / step) * step;
}

float3 WrapSafeRate(float3 rate, float period)
{
  const float step = period / TIME_WRAP_AROUND;
  return round(rate / step) * step;
}

float4 WrapSafeRate(float4 rate, float period)
{
  const float step = period / TIME_WRAP_AROUND;
  return round(rate / step) * step;
}

// Rounds an angular frequency (radians per second, as in sin(GlobalTime * frequency)) so that the sine / cosine does not jump when the time wraps around.
float WrapSafeAngularFrequency(float frequency)
{
  return WrapSafeRate(frequency, 2.0f * PI);
}

float2 WrapSafeAngularFrequency(float2 frequency)
{
  return WrapSafeRate(frequency, 2.0f * PI);
}

float3 WrapSafeAngularFrequency(float3 frequency)
{
  return WrapSafeRate(frequency, 2.0f * PI);
}

float4 WrapSafeAngularFrequency(float4 frequency)
{
  return WrapSafeRate(frequency, 2.0f * PI);
}
#endif

#include "CameraConstantsAccess.h"
