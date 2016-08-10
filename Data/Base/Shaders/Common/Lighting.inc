#pragma once

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Common/BRDF.inc>

float3 CalculateLighting(MaterialData matData)
{
	float3 totalLight = 0.0f;
	
	// fake sun light
	float3 N = matData.worldNormal;
	float3 L = normalize(float3(0.1, 0.3, 1.0f));
	float3 V = normalize(CameraPosition - matData.worldPosition);
	
	float attenuation = saturate(dot(N, L)) * (1.0f / PI);
	float3 lightColor = float3(2.0f, 1.8f, 1.7f) * 10.0f;
	
	totalLight += DefaultShading(matData, L, V, float2(1.0f, 1.0f)) * lightColor * attenuation;	
	
	// fake ambient
	float3 ambientBottom = float3(0.1f, 0.1f, 0.15f);
	float3 ambientTop = float3(0.2f, 0.2f, 0.3f);
	float3 ambientLight = lerp(ambientBottom, ambientTop, matData.worldNormal.z * 0.5f + 0.5f);
	
	totalLight += matData.diffuseColor * ambientLight;
	
	return totalLight;
}
