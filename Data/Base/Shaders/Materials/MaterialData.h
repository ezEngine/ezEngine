#pragma once

#include <Shaders/Common/Common.h>

struct ezMaterialData
{
  float3 worldPosition;
  float3 worldNormal;
  float3 vertexNormal;
  float3 normalizedViewVector;
  float viewDistance;

  float3 diffuseColor;
  float3 specularColor;
  float3 emissiveColor;
  float4 refractionColor;
  float roughness;
  float occlusion;
  float opacity;

  float3 subsurfaceColor;
  float subsurfaceScatterPower;
  float subsurfaceShadowFalloff;
};
