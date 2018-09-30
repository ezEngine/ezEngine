#pragma once

#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME

  // No need to do alpha test again if we have a depth prepass
  #if defined(FORWARD_PASS_WRITE_DEPTH) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR)
    #if FORWARD_PASS_WRITE_DEPTH == TRUE
      #define USE_ALPHA_TEST
    #endif
  #else
    #define USE_ALPHA_TEST
  #endif

  #if defined(USE_ALPHA_TEST) && defined(MSAA)
    #if MSAA == TRUE
      #define USE_ALPHA_TEST_SUPER_SAMPLING
    #endif
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
#if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.RenderTargetArrayIndex;
#endif

  G.Input = Input;
  #if defined(CUSTOM_GLOBALS)
    FillCustomGlobals();
  #endif

  PS_OUT Output;

  #if defined(USE_ALPHA_TEST)
    uint coverage = CalculateCoverage();
    if (coverage == 0)
    {
      discard;
    }

    #if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
      Output.Coverage = coverage;
    #endif
  #endif

  ezMaterialData matData = FillMaterialData();

  ezPerClusterData clusterData = GetClusterData(Input.Position.xyw);
  uint gameObjectId = GetInstanceData().GameObjectID;

  #if defined(USE_DECALS)
    ApplyDecals(matData, clusterData, gameObjectId);
  #endif

  #if RENDER_PASS == RENDER_PASS_EDITOR
    if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
    {
      matData.diffuseColor = 0.5;
      matData.specularColor = 0.0;
    }
  #endif

  #if SHADING_MODE == SHADING_MODE_LIT
    #if BLEND_MODE == BLEND_MODE_OPAQUE || BLEND_MODE == BLEND_MODE_MASKED
      bool applySSAO = true;
    #else
      bool applySSAO = false;
    #endif

    AccumulatedLight light = CalculateLighting(matData, clusterData, Input.Position.xyw, applySSAO);
  #else
    AccumulatedLight light = InitializeLight(matData.diffuseColor, 0.0f);
  #endif

  #if BLEND_MODE != BLEND_MODE_OPAQUE && BLEND_MODE != BLEND_MODE_MASKED
    if (matData.opacity > 0.01f)
    {
      light.specularLight /= matData.opacity;
    }

    #if defined(USE_MATERIAL_REFRACTION)
      ApplyRefraction(matData, light);
    #endif
  #endif

  float3 litColor = light.diffuseLight + light.specularLight;
  litColor += matData.emissiveColor;

  #if RENDER_PASS == RENDER_PASS_FORWARD
    #if defined(USE_FOG)
      #if BLEND_MODE == BLEND_MODE_ADDITIVE
        matData.opacity *= GetFogAmount(matData.worldPosition);
      #elif BLEND_MODE == BLEND_MODE_MODULATE
        litColor = lerp(1.0, litColor, GetFogAmount(matData.worldPosition));
      #else
        litColor = ApplyFog(litColor, matData.worldPosition);
      #endif
    #endif

    Output.Color = float4(litColor, matData.opacity);

  #elif RENDER_PASS == RENDER_PASS_EDITOR
    if (RenderPass == EDITOR_RENDER_PASS_LIT_ONLY)
    {
      Output.Color = float4(SrgbToLinear(litColor * Exposure), matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT || RenderPass == EDITOR_RENDER_PASS_DECAL_COUNT)
    {
      float lightCount = RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT ? GET_LIGHT_INDEX(clusterData.counts) : GET_DECAL_INDEX(clusterData.counts);
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
      Output.Color = float4(matData.diffuseColor, matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
    {
      Output.Color = float4(matData.diffuseColor, matData.opacity);

      float luminance = GetLuminance(matData.diffuseColor);
      if (luminance < 0.017) // 40 srgb
      {
        Output.Color = float4(1, 0, 1, matData.opacity);
      }
      else if (luminance > 0.9) // 243 srgb
      {
        Output.Color = float4(0, 1, 0, matData.opacity);
      }
    }
    else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_COLOR)
    {
      Output.Color = float4(matData.specularColor, matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_EMISSIVE_COLOR)
    {
      Output.Color = float4(matData.emissiveColor, matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_ROUGHNESS)
    {
      Output.Color = float4(SrgbToLinear(matData.roughness), matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
    {
      float ssao = SampleSSAO(Input.Position.xyw);
      float occlusion = matData.occlusion * ssao;

      Output.Color = float4(SrgbToLinear(occlusion), matData.opacity);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
    {
      float depth = Input.Position.w * ClipPlanes.z;
      Output.Color = float4(SrgbToLinear(depth), 1);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC)
    {
      Output.Color = ColorizeGameObjectId(gameObjectId);
    }
    else if (RenderPass == EDITOR_RENDER_PASS_BONE_WEIGHTS)
    {
      #if defined(USE_DEBUG_INTERPOLATOR)
        Output.Color = Input.DebugInterpolator;
      #else
        Output.Color = float4(0.05, 0.05, 0.1, 1);
      #endif
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

  #elif (RENDER_PASS == RENDER_PASS_PICKING || RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME)
    Output.Color = RGBA8ToFloat4(gameObjectId);

  #elif RENDER_PASS == RENDER_PASS_DEPTH_ONLY

  #else
    Output.Color = float4(litColor, matData.opacity);
    #error "RENDER_PASS uses undefined value."
  #endif

  return Output;
}
