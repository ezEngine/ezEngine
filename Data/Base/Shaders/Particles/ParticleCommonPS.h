#pragma once

#include <Shaders/Materials/MaterialInterpolator.h>
#include <Shaders/Particles/ParticleSystemConstants.h>

#if SHADING_QUALITY == SHADING_QUALITY_NORMAL
#  include <Shaders/Common/Lighting.h>
#elif SHADING_QUALITY == SHADING_QUALITY_SIMPLIFIED
#  include <Shaders/Common/LightingSimplified.h>
#else
#  error "Unknown shading quality configuration."
#endif

float CalcCloseToGeometryFadeOut(float4 position, float fadeOutDistance)
{
  return DepthFade(position.xyw, fadeOutDistance);
}

float CalcCloseToCameraFadeOut(float4 position)
{
  return saturate(position.w / 0.5);
}

float CalcProximityFadeOut(float4 position, float fadeOutDistance)
{
  return CalcCloseToGeometryFadeOut(position, fadeOutDistance) * CalcCloseToCameraFadeOut(position);
}
