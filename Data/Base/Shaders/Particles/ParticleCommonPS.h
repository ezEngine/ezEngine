#pragma once

#define USE_TEXCOORD0
#define USE_COLOR

#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Particles/ParticleSystemConstants.h>
#include <Shaders/Common/Lighting.h>

float CalcCloseToGeometryFadeOut(float4 position)
{
  return DepthFade(position.xyw, 0.1);
}

float CalcCloseToCameraFadeOut(float4 position)
{
  return saturate(position.w / 0.5);
}

float CalcProximityFadeOut(float4 position)
{
  return CalcCloseToGeometryFadeOut(position) * CalcCloseToCameraFadeOut(position);
}
