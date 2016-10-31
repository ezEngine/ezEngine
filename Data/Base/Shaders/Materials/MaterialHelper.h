#pragma once

#include <Shaders/Common/Common.h>
#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialData.h>
#include <Shaders/Materials/MaterialInterpolator.h>

float3 GetNormal(PS_IN Input);

#if defined(USE_SIMPLE_MATERIAL_MODEL)
	float3 GetBaseColor(PS_IN Input);
	float GetMetallic(PS_IN Input);
	float GetReflectance(PS_IN Input);
#else
	float3 GetDiffuseColor(PS_IN Input);
	float3 GetSpecularColor(PS_IN Input);
#endif

#if defined(USE_MATERIAL_EMISSIVE)
  float3 GetEmissiveColor(PS_IN Input);
#endif

float GetRoughness(PS_IN Input);
float GetOpacity(PS_IN Input);

#if defined(USE_MATERIAL_OCCLUSION)
  float GetOcclusion(PS_IN Input);
#endif

ezPerInstanceData GetInstanceData(PS_IN Input)
{
	#if INSTANCING
		return perInstanceData[Input.InstanceID];
	#else
		return perInstanceData;
	#endif
}

uint CalculateCoverage(PS_IN Input)
{
	#if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
		uint coverage = 0;

		for (uint i = 0; i < NumMsaaSamples; ++i)
		{
			PS_IN InputCopy = Input;
			InputCopy.TexCoords.xy = EvaluateAttributeAtSample(Input.TexCoords.xy, i);

			float opacity = GetOpacity(InputCopy);
			coverage |= (opacity > 0.0) ? (1 << i) : 0;
		}

		return coverage;
	#else
		return GetOpacity(Input) > 0.0;
	#endif
}

ezMaterialData FillMaterialData(PS_IN Input)
{
	ezMaterialData matData;

	#if defined(USE_WORLDPOS)
		matData.worldPosition = Input.WorldPosition;
	#else
		matData.worldPosition = float3(0.0, 0.0, 0.0);
	#endif
  
  matData.normalizedViewVector = normalize(CameraPosition - matData.worldPosition);

	float3 worldNormal = normalize(GetNormal(Input));
	#if TWO_SIDED == TRUE
		matData.worldNormal = Input.FrontFace ? worldNormal : -worldNormal;
	#else
		matData.worldNormal = worldNormal;
	#endif

	#if defined(USE_SIMPLE_MATERIAL_MODEL)
		float3 baseColor = GetBaseColor(Input);
		float metallic = GetMetallic(Input);
		float reflectance = GetReflectance(Input);
		float f0 = 0.16f * reflectance * reflectance;

		matData.diffuseColor = lerp(baseColor, 0.0f, metallic);
		matData.specularColor = lerp(float3(f0, f0, f0), baseColor, metallic);

	#else
		matData.diffuseColor = GetDiffuseColor(Input);
		matData.specularColor = GetSpecularColor(Input);
	#endif
  
  #if defined(USE_MATERIAL_EMISSIVE)
    matData.emissiveColor = GetEmissiveColor(Input);  
  #else
    matData.emissiveColor = 0.0f;
  #endif
  
  matData.roughness = max(GetRoughness(Input), 0.04f);
  
  #if defined(USE_MATERIAL_OCCLUSION)
    #if defined(USE_NORMAL)
      float occlusionFade = saturate(dot(Input.Normal, matData.normalizedViewVector));
    #else
      float occlusionFade = 1.0f;
    #endif
    matData.occlusion = lerp(1.0f, GetOcclusion(Input), occlusionFade);
  #else
    matData.occlusion = 1.0f;
  #endif

	return matData;
}

#if defined(USE_NORMAL) && defined(USE_TANGENT)
  float3 TangentToWorldSpace(float3 normalTS, PS_IN Input)
  {
    return normalTS.x * Input.Tangent + normalTS.y * Input.BiTangent + normalTS.z * Input.Normal;
  }
#endif
