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

float CalculateFogAmount(float3 worldPosition)
{
  return 1.0f;
}

float3 ApplyFogColor(float3 color, float3 worldPosition, float fogAmount)
{
  return color;
}

float3 CalculateAndApplyFog(float3 color, float3 worldPosition)
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

// The simplified path has no scene color or scene depth texture bound, so everything that would
// read them returns a neutral value. These exist because the visual shader nodes in Utils.ddl and
// the refraction path in MaterialPixelShader.h can be used at any shading quality.

float SampleSceneDepth(float2 screenPosition)
{
  return 0.0f;
}

float3 SampleScenePosition(float2 screenPosition)
{
  return float3(0.0, 0.0, 0.0);
}

float4 CalculateRefraction(float3 worldPosition, float4 screenPosition, float3 worldNormal, float2 distortion, float newOpacity, out float2 refractedScreenPosition)
{
  refractedScreenPosition = screenPosition.xy;
  return float4(SampleSceneColor(screenPosition.xy), newOpacity);
}

void ApplyRefraction(inout ezMaterialData matData, inout AccumulatedLight light)
{
  light.diffuseLight = lerp(matData.refractionColor.rgb, light.diffuseLight, matData.opacity);
  matData.opacity = matData.refractionColor.a;
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
