#pragma once

#define USE_WORLDPOS

#include <Shaders/Materials/Lighting.inc>
#include <Shaders/Materials/MaterialInterpolator.inc>

float3 GetNormal(PS_IN Input);

#if defined(USE_SIMPLE_MATERIAL_MODEL)
	float3 GetDiffuseColor(PS_IN Input);
	float GetMetallic(PS_IN Input);
	float GetReflectance(PS_IN Input);
#else
	float3 GetDiffuseColor(PS_IN Input);
	float3 GetSpecularColor(PS_IN Input);
#endif
	
float GetRoughness(PS_IN Input);
float GetOpacity(PS_IN Input);


MaterialData FillMaterialData(PS_IN Input)
{
	MaterialData matData;
	matData.worldPosition = Input.WorldPosition;
	
	float3 worldNormal = normalize(GetNormal(Input));
	#if TWO_SIDED == TRUE
		matData.worldNormal = Input.FrontFace ? worldNormal : -worldNormal;
	#else
		matData.worldNormal = worldNormal;
	#endif
	
	#if defined(USE_SIMPLE_MATERIAL_MODEL)
		float3 baseColor = GetDiffuseColor(Input);
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
	
	#if RENDER_PASS != RENDER_PASS_DEPTH_ONLY
		#if RENDER_PASS == RENDER_PASS_FORWARD
			#if defined(SHADING_MODE) && SHADING_MODE == SHADING_MODE_LIT
				MaterialData matData = FillMaterialData(Input);
			
				float3 finalColor = CalculateLighting(matData);
				return float4(finalColor, opacity);
			#else
				#if defined(USE_SIMPLE_MATERIAL_MODEL)
					float3 finalColor = GetDiffuseColor(Input);
				#else
					float3 finalColor = GetDiffuseColor(Input);
				#endif
				return float4(finalColor, opacity);
			#endif
		
		#elif RENDER_PASS == RENDER_PASS_EDITOR
			MaterialData matData = FillMaterialData(Input);
		
			if (RenderPass == EDITOR_RENDER_PASS_PICKING)
			{
				return uintToFloat4(GameObjectID);
			}
			else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
			{
				// Would need to be gamma corrected
				#if defined(USE_TEXCOORD0)
					return float4(frac(Input.TexCoords.xy), 0, 1);
				#else
					return float4(0, 0, 0, 1);
				#endif
			}
			else if (RenderPass == EDITOR_RENDER_PASS_NORMALS)
			{
				// Would need to be gamma corrected
				return float4(matData.worldNormal * 0.5 + 0.5, 1);
			}
			else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
			{
				float depth = 1.0 - saturate(Input.Position.z / Input.Position.w);
				depth = depth * depth * depth * depth;
				return float4(depth, depth, depth, 1);
			}
			else
			{
				return float4(1.0f, 0.0f, 1.0f, 1.0f);
			}
		
		#elif RENDER_PASS == RENDER_PASS_WIREFRAME
			if (RenderPass == WIREFRAME_RENDER_PASS_PICKING)
			{
				return uintToFloat4(GameObjectID);
			}
			else if (RenderPass == WIREFRAME_RENDER_PASS_MONOCHROME)
			{
				return float4(0.4f, 0.4f, 0.4f, 1.0f);
			}
			else
			{
				#if defined(USE_SIMPLE_MATERIAL_MODEL)
					float3 finalColor = GetDiffuseColor(Input);
				#else
					float3 finalColor = GetDiffuseColor(Input);
				#endif
				return float4(finalColor, 1.0f);
			}
		#endif
	#endif
}
