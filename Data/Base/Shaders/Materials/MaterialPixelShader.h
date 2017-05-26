#pragma once

#if !defined(RENDER_PASS) || !defined(BLEND_MODE)
  #error "RENDER_PASS and BLEND_MODE permutations must be defined"
#endif

#define USE_WORLDPOS

#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME
  #define USE_ALPHA_TEST
  
  #if MSAA
    #define USE_ALPHA_TEST_SUPER_SAMPLING
  #endif
#endif

#include <Shaders/Common/Lighting.h>
#include <Shaders/Materials/MaterialHelper.h>

struct PS_OUT
{
	#if RENDER_PASS != RENDER_PASS_DEPTH_ONLY
		float4 Color : SV_Target;
	#endif

	#if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
		uint Coverage : SV_Coverage;
	#endif
};

PS_OUT main(PS_IN Input)
{
  PS_OUT Output;
  
  float opacity = 1.0f;

  #if defined(USE_ALPHA_TEST)
    uint coverage = CalculateCoverage(Input);
    if (coverage == 0)
    { 
      discard;
    }
      
    #if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
      Output.Coverage = coverage;
    #endif
    
  #elif BLEND_MODE != BLEND_MODE_OPAQUE
    opacity = GetOpacity(Input);
  #endif

  ezMaterialData matData = FillMaterialData(Input);

  #if RENDER_PASS == RENDER_PASS_EDITOR
    if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
    {
      matData.diffuseColor = 0.5;
      matData.specularColor = 0.0;
    }
  #endif
  
  ezPerClusterData clusterData = GetClusterData(Input.Position.xyw);
  
  #if defined(SHADING_MODE) && SHADING_MODE == SHADING_MODE_LIT
    float3 litColor = CalculateLighting(matData, clusterData, Input.Position.xyw);
  #else
    float3 litColor = matData.diffuseColor;
  #endif
  
  litColor += matData.emissiveColor;

  #if RENDER_PASS == RENDER_PASS_FORWARD
    Output.Color = float4(litColor, opacity);
    
  #elif RENDER_PASS == RENDER_PASS_EDITOR
    if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
    {
      Output.Color =  float4(SrgbToLinear(litColor * Exposure), opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT)
    {
      float lightCount = clusterData.counts;
      if (lightCount == 0)
      {
        Output.Color = float4(0, 0, 0, 1);
      }
      else
      {
        float x = (lightCount - 1) / 16;
        float r = saturate(x);
        float g = saturate(2 - x);
        
        Output.Color = float4(r, g, 0, 1);
      }
    }
    else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
    {
      #if defined(USE_TEXCOORD0)
        Output.Color = float4(SrgbToLinear(float3(frac(Input.TexCoords.xy), 0)), 1);
      #else
        Output.Color = float4(0, 0, 0, 1);
      #endif
    }
    else if (RenderPass == EDITOR_RENDER_PASS_PIXEL_NORMALS)
    {
      Output.Color = float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_NORMALS)
    {
      #if defined(USE_NORMAL)
        Output.Color = float4(SrgbToLinear(normalize(Input.Normal) * 0.5 + 0.5), 1);
      #else
        Output.Color = float4(0, 0, 0, 1);
      #endif
    }
    else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_TANGENTS)
    {
      #if defined(USE_TANGENT)
        Output.Color = float4(SrgbToLinear(normalize(Input.Tangent) * 0.5 + 0.5), 1);
      #else
        Output.Color = float4(0, 0, 0, 1);
      #endif
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR)
    {
      Output.Color = float4(matData.diffuseColor, opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
    {
      Output.Color = float4(matData.diffuseColor, opacity);
      
      float luminance = GetLuminance(matData.diffuseColor);
      if (luminance < 0.017) // 40 srgb
      {
        Output.Color = float4(1, 0, 1, opacity);
      }
      else if (luminance > 0.9) // 243 srgb
      {
        Output.Color = float4(0, 1, 0, opacity);
      }
    }
    else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_COLOR)
    {
      Output.Color = float4(matData.specularColor, opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_EMISSIVE_COLOR)
    {
      Output.Color = float4(matData.emissiveColor, opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
    {
      Output.Color = float4(SrgbToLinear(matData.roughness), opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
    {
      float occlusion = matData.occlusion;
      
      #if USE_SSAO
        float ssao = SSAOTexture.SampleLevel(PointClampSampler, Input.Position.xy * ViewportSize.zw, 0.0f).r;
        occlusion = min(occlusion, ssao);
      #endif
      
      Output.Color = float4(SrgbToLinear(occlusion), opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
    {
      float depth = Input.Position.w * ClipPlanes.z;
      Output.Color = float4(SrgbToLinear(depth), 1);
    }
    else
    {
      Output.Color = float4(1.0f, 0.0f, 1.0f, 1.0f);
    }

  #elif RENDER_PASS == RENDER_PASS_WIREFRAME
    if (RenderPass == WIREFRAME_RENDER_PASS_MONOCHROME)
    {
      Output.Color = float4(0.4f, 0.4f, 0.4f, 1.0f);
    }
    else
    {
      Output.Color = float4(matData.diffuseColor, 1.0f);
    }

  #elif RENDER_PASS == RENDER_PASS_PICKING
    Output.Color = RGBA8ToFloat4(GetInstanceData(Input).GameObjectID);

  #endif

  return Output;
}
