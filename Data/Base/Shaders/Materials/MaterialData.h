#pragma once

#include <Shaders/Common/Common.h>

struct ezMaterialData
{
	float3 worldPosition;
	float3 worldNormal;

	float3 diffuseColor;
	float3 specularColor;
	float roughness;
};
