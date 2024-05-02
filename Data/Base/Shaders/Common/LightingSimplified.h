#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_SIMPLIFIED
#  error "Functions in LightDataSimplified.h are only for SIMPLIFIED shading quality. Include LightData.h instead."
#endif

#include <Shaders/Common/AmbientCubeBasis.h>
#include <Shaders/Common/BRDF.h>
#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/LightDataSimplified.h>

TextureCubeArray ReflectionSpecularTexture;
Texture2D SkyIrradianceTexture;

///////////////////////////////////////////////////////////////////////////////////

float GetFogAmount(float3 worldPosition)
{
  return 1.0f;
}

float3 ApplyFog(float3 color, float3 worldPosition, float fogAmount)
{
  return color;
}

float3 ApplyFog(float3 color, float3 worldPosition)
{
  return color;
}

float DepthFade(float3 screenPosition, float fadeDistance)
{
  return 1.0f;
}

float3 SampleSceneColor(float2 screenPosition)
{
  return float3(1.0, 1.0, 1.0);
}

AccumulatedLight CalculateLightingSimplified(ezMaterialData matData)
{
  AccumulatedLight totalLight = InitializeLight(0.0f, 0.0f);

  float occlusion = matData.occlusion;

  // sky light in ambient cube basis
  float3 skyLight = EvaluateAmbientCube(SkyIrradianceTexture, SkyIrradianceIndex, matData.worldNormal).rgb;
  totalLight.diffuseLight += matData.diffuseColor * skyLight * occlusion;

  return totalLight;
}
