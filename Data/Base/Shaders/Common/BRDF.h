#pragma once

#include <Shaders/Materials/MaterialData.h>

// TODO:
// check roughness^2 vs. roughness^4
// try different visibility function (SmithGGXCorrelated)

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

float VisibilitySmithJointApprox( float roughness, float NdotV, float NdotL )
{
	float a = roughness * roughness;
	float lambdaV = NdotL * ( NdotV * ( 1.0f - a ) + a );
	float lambdaL = NdotV * ( NdotL * ( 1.0f - a ) + a );
	return 0.5f / ( lambdaV + lambdaL );
}

float3 FresnelSchlick( float3 f0, float u )
{
	return f0 + (1.0f - f0) * pow(1.0f - u, 5.0f);
}

// note that N.L * 1/PI is already included in the light attenuation which is applied later
float3 DefaultShading( ezMaterialData matData, float3 L, float3 V, float2 diffSpecEnergy )
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

	return diffuse * diffSpecEnergy.x + F * (D * Vis * diffSpecEnergy.y);
}
