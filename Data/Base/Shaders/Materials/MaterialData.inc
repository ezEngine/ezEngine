#pragma once

#include <Shaders/Common/Common.inc>

struct MaterialData
{
	float3 worldPosition;
	float3 worldNormal;
	
	float3 diffuseColor;
	float3 specularColor;
	float roughness;	
};
