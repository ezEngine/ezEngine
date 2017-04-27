#pragma once

#define USE_WORLDPOS

#include <Shaders/Common/Lighting.h>
#include <Shaders/Materials/MaterialHelper.h>

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
  
  ezPerClusterData clusterData = GetClusterData(Input.Position.xyw);

  float3 litColor = CalculateLighting(matData, clusterData, Input.Position.xyw);
  litColor += matData.emissiveColor;

  #if RENDER_PASS == RENDER_PASS_FORWARD
    #if defined(SHADING_MODE) && SHADING_MODE == SHADING_MODE_LIT
      //litColor = ApplyFog(litColor, matData.worldPosition);
    
      return float4(litColor, opacity);
    #else
      return float4(matData.diffuseColor + matData.emissiveColor, opacity);
    #endif

  #elif RENDER_PASS == RENDER_PASS_EDITOR
    if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
    {
      return float4(SrgbToLinear(litColor * Exposure), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT)
    {
      float lightCount = clusterData.counts;
      if (lightCount == 0)
        return float4(0, 0, 0, 1);
      
      float x = (lightCount - 1) / 16;
      float r = saturate(x);
      float g = saturate(2 - x);
      
      return float4(r, g, 0, 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
    {
      #if defined(USE_TEXCOORD0)
        return float4(SrgbToLinear(float3(frac(Input.TexCoords.xy), 0)), 1);
      #else
        return float4(0, 0, 0, 1);
      #endif
    }
    else if (RenderPass == EDITOR_RENDER_PASS_PIXEL_NORMALS)
    {
      return float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_NORMALS)
    {
      #if defined(USE_NORMAL)
        return float4(SrgbToLinear(normalize(Input.Normal) * 0.5 + 0.5), 1);
      #else
        return float4(0, 0, 0, 0);
      #endif
    }
    else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_TANGENTS)
    {
      #if defined(USE_TANGENT)
        return float4(SrgbToLinear(normalize(Input.Tangent) * 0.5 + 0.5), 1);
      #else
        return float4(0, 0, 0, 0);
      #endif
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
    else if (RenderPass == EDITOR_RENDER_PASS_EMISSIVE_COLOR)
    {
      return float4(matData.emissiveColor, 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
    {
      return float4(SrgbToLinear(matData.roughness), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
    {
      return float4(SrgbToLinear(matData.occlusion), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
    {
      float depth = Input.Position.w * ClipPlanes.z;
      return float4(SrgbToLinear(depth), 1);
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
