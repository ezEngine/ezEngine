#pragma once

#include <Shaders/Common/Common.h>

struct ezMaterialData
{
  float3 worldPosition;
  float3 worldNormal;
  float3 vertexNormal;

  float3 diffuseColor;
  float3 specularColor;
  float3 emissiveColor;
  float4 refractionColor;
  float roughness;
  float perceptualRoughness;
  float metalness;
  float occlusion;
  float opacity;

#if defined(USE_MATERIAL_SPECULAR_ANISOTROPIC)
  float anisotropic;
  float anisotropicRotation;
#endif

#if defined(USE_MATERIAL_SPECULAR_CLEARCOAT)
  float clearcoat;
  float clearcoatRoughness;
  float3 clearcoatNormal;
#endif

#if defined(USE_MATERIAL_SPECULAR_SHEEN)
  float sheen;
  float sheenTintFactor;
#endif

  float3 subsurfaceColor;
  float subsurfaceScatterPower;
  float subsurfaceShadowFalloff;
};
