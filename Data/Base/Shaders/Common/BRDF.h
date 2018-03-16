#pragma once

#include <Shaders/Materials/MaterialData.h>

// TODO:
// check roughness^2 vs. roughness^4
// try different visibility function (SmithGGXCorrelated)

struct AccumulatedLight
{
  float3 diffuseLight;
  float3 specularLight;
};

AccumulatedLight InitializeLight(float3 diff, float3 spec)
{
  AccumulatedLight result;
  result.diffuseLight = diff;
  result.specularLight = spec;
  return result;
}

void AccumulateLight(inout AccumulatedLight result, AccumulatedLight light)
{
  result.diffuseLight += light.diffuseLight;
  result.specularLight += light.specularLight;
}

void AccumulateLight(inout AccumulatedLight result, AccumulatedLight light, float3 color)
{
  result.diffuseLight += light.diffuseLight * color;
  result.specularLight += light.specularLight * color;
}

///////////////////////////////////////////////////////////////////////////////////

float3 DiffuseLambert( float3 diffuseColor )
{
  return diffuseColor;
}

float SpecularGGX( float roughness, float NdotH )
{
  // mad friendly reformulation of:
  //
  //              m^2
  // --------------------------------
  // PI * ((N.H)^2 * (m^2 - 1) + 1)^2

  float m = roughness * roughness;
  float m2 = m * m;
  float f = ( NdotH * m2 - NdotH ) * NdotH + 1.0f;
  return m2 / ( f * f );
}

float VisibilitySmithCorrelated( float roughness, float NdotV, float NdotL )
{
  float a = roughness * roughness;
  float lambdaV = NdotL * sqrt((-NdotV * a + NdotV) * NdotV + a);
  float lambdaL = NdotV * sqrt((-NdotL * a + NdotL) * NdotL + a);
  return 0.5f / ( lambdaV + lambdaL );
}

float VisibilitySmithJointApprox( float roughness, float NdotV, float NdotL )
{
  float a = roughness * roughness;
  float b = 1.0f - a;
  float lambdaV = NdotL * ( NdotV * b + a );
  float lambdaL = NdotV * ( NdotL * b + a );
  return 0.5f / ( lambdaV + lambdaL );
}

float3 FresnelSchlick( float3 specularColor, float u )
{
  float f = 1.0f - u;
  float ff = f * f;
  float f5 = ff * ff * f;
  
  // specularColor below 2% is considered to be shadowing
  return saturate( 50.0 * specularColor.g ) * f5 + (1 - f5) * specularColor;
}

// note that N.L * 1/PI is already included in the light attenuation which is applied later
AccumulatedLight DefaultShading( ezMaterialData matData, float3 L, float3 V, float2 diffSpecEnergy )
{
  float3 N = matData.worldNormal;
  float3 H = normalize(V + L);
  float NdotL = saturate( dot(N, L) );
  float NdotV = max( dot(N, V), 1e-5f );
  float NdotH = saturate( dot(N, H) );
  float VdotH = saturate( dot(V, H) );

  // Cook-Torrance microfacet BRDF
  //
  //    D * G * F                                  G
  //  ------------- = D * Vis * F with Vis = -------------
  //  4 * N.L * N.V                          4 * N.L * N.V

  float D = SpecularGGX( matData.roughness, NdotH );
  float Vis = VisibilitySmithJointApprox( matData.roughness, NdotV, NdotL );
  float3 F = FresnelSchlick( matData.specularColor, VdotH );

  float3 diffuse = DiffuseLambert( matData.diffuseColor );
  
  return InitializeLight(diffuse * diffSpecEnergy.x, F * (D * Vis * diffSpecEnergy.y));
}
