#pragma once

#define USE_WORLDPOS

#include <Shaders/Common/Lighting.h>
#include <Shaders/Common/ObjectConstants.h>
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

float GetRoughness(PS_IN Input);
float GetOpacity(PS_IN Input);

ezPerInstanceData GetInstanceData(PS_IN Input)
{
	#if INSTANCING
		return perInstanceData[Input.InstanceID];
	#else
		return perInstanceData;
	#endif
}

ezMaterialData FillMaterialData(PS_IN Input)
{
	ezMaterialData matData;
	matData.worldPosition = Input.WorldPosition;

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

	matData.roughness = max(GetRoughness(Input), 0.04f);

	return matData;
}


#if !defined(RENDER_PASS) || !defined(BLEND_MODE)
	#error "RENDER_PASS and BLEND_MODE permutations must be defined"
#endif

#if RENDER_PASS == RENDER_PASS_DEPTH_ONLY
	void main(PS_IN Input)
#else
	float4 main(PS_IN Input) : SV_Target
#endif
{
	float opacity = 1.0f;

	#if BLEND_MODE != BLEND_MODE_OPAQUE
		opacity = GetOpacity(Input);

		#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME
			clip(opacity);
		#endif
	#endif

	ezMaterialData matData = FillMaterialData(Input);

	#if RENDER_PASS == RENDER_PASS_EDITOR
		if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
		{
			matData.diffuseColor = 0.5;
			matData.specularColor = 0.0;
		}
	#endif

	float3 litColor = CalculateLighting(matData);

	#if RENDER_PASS == RENDER_PASS_FORWARD
		#if defined(SHADING_MODE) && SHADING_MODE == SHADING_MODE_LIT
			return float4(litColor, opacity);
		#else
			return float4(matData.diffuseColor, opacity);
		#endif

	#elif RENDER_PASS == RENDER_PASS_EDITOR
		if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
		{
			return float4(litColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
		{
			#if defined(USE_TEXCOORD0)
				return float4(SrgbToLinear(float3(frac(Input.TexCoords.xy), 0)), 1);
			#else
				return float4(0, 0, 0, 1);
			#endif
		}
		else if (RenderPass == EDITOR_RENDER_PASS_NORMALS)
		{
			return float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR)
		{
			return float4(matData.diffuseColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
		{
			float luminance = GetLuminance(matData.diffuseColor);
			if (luminance < 0.017) // 40 srgb
			{
				return float4(1, 0, 1, 1);
			}
			else if (luminance > 0.9) // 243 srgb
			{
				return float4(0, 1, 0, 1);
			}

			return float4(matData.diffuseColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_COLOR)
		{
			return float4(matData.specularColor, 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
		{
			return float4(SrgbToLinear(matData.roughness), 1);
		}
		else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
		{
			// todo proper linearization
			float depth = 1.0 - saturate(Input.Position.z / Input.Position.w);
			depth = depth * depth * depth * depth;
			return float4(depth, depth, depth, 1);
		}
		else
		{
			return float4(1.0f, 0.0f, 1.0f, 1.0f);
		}

	#elif RENDER_PASS == RENDER_PASS_WIREFRAME
		if (RenderPass == WIREFRAME_RENDER_PASS_MONOCHROME)
		{
			return float4(0.4f, 0.4f, 0.4f, 1.0f);
		}
		else
		{
			return float4(matData.diffuseColor, 1.0f);
		}

	#elif RENDER_PASS == RENDER_PASS_PICKING
		return RGBA8ToFloat4(GetInstanceData(Input).GameObjectID);

	#endif
}
