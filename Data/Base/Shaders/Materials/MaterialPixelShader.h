#pragma once

#if !defined(RENDER_PASS) || !defined(BLEND_MODE)
#  error "RENDER_PASS and BLEND_MODE permutations must be defined"
#endif

#define USE_WORLDPOS

#if (BLEND_MODE == BLEND_MODE_MASKED || BLEND_MODE == BLEND_MODE_DITHERED) && RENDER_PASS != RENDER_PASS_WIREFRAME

// No need to do alpha test again if we have a depth prepass
#  if defined(FORWARD_PASS_WRITE_DEPTH) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR)
#    if FORWARD_PASS_WRITE_DEPTH == TRUE
#      define USE_ALPHA_TEST
#      if (BLEND_MODE == BLEND_MODE_DITHERED)
#        define USE_DITHERING
#      endif
#    endif
#  else
#    define USE_ALPHA_TEST
#    if (BLEND_MODE == BLEND_MODE_DITHERED)
#      define USE_DITHERING
#    endif
#  endif

#  if defined(USE_ALPHA_TEST) && defined(MSAA)
#    if MSAA == TRUE
#      define WRITE_COVERAGE
#    endif
#  endif

#endif

#if BLEND_MODE == BLEND_MODE_TRANSPARENT && RENDER_PASS == RENDER_PASS_DEPTH_ONLY
#  define USE_ALPHA_TEST
#  define USE_DITHERING
#  if defined(MSAA)
#    if MSAA == TRUE
#      define WRITE_COVERAGE
#    endif
#  endif
#endif

#if SHADING_QUALITY == SHADING_QUALITY_NORMAL
#  include <Shaders/Common/Lighting.h>
#elif SHADING_QUALITY == SHADING_QUALITY_SIMPLIFIED
#  include <Shaders/Common/LightingSimplified.h>
#else
#  error "Unknown shading quality configuration."
#endif

#include <Shaders/Materials/MaterialHelper.h>

struct PS_OUT
{
#if RENDER_PASS != RENDER_PASS_DEPTH_ONLY
  float4 Color : SV_Target;
#endif

#if defined(WRITE_COVERAGE)
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

#  if defined(WRITE_COVERAGE)
  Output.Coverage = coverage;
#  endif
#endif

  ezMaterialData matData = FillMaterialData();

  uint gameObjectId = GetInstanceData().GameObjectID;
#if SHADING_QUALITY == SHADING_QUALITY_NORMAL
  ezPerClusterData clusterData = GetClusterData(Input.Position.xyw);
#  if defined(USE_DECALS)
  ApplyDecals(matData, clusterData, gameObjectId);
#  endif
#endif

#if RENDER_PASS == RENDER_PASS_EDITOR
  if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY)
  {
    matData.diffuseColor = 0.5;
  }
#endif

#if SHADING_MODE == SHADING_MODE_LIT
#  if SHADING_QUALITY == SHADING_QUALITY_NORMAL
#    if BLEND_MODE == BLEND_MODE_OPAQUE || BLEND_MODE == BLEND_MODE_MASKED || BLEND_MODE == BLEND_MODE_DITHERED
  bool applySSAO = true;
#    else
  bool applySSAO = false;
#    endif

  AccumulatedLight light = CalculateLighting(matData, clusterData, Input.Position.xyw, applySSAO);
#  elif SHADING_QUALITY == SHADING_QUALITY_SIMPLIFIED
  AccumulatedLight light = CalculateLightingSimplified(matData);
#  endif
#else
  AccumulatedLight light = InitializeLight(matData.diffuseColor, 0.0f);
#endif

#if BLEND_MODE != BLEND_MODE_OPAQUE && BLEND_MODE != BLEND_MODE_MASKED
#  if defined(USE_MATERIAL_REFRACTION)
  ApplyRefraction(matData, light);
#  endif

  float specularNormalization = lerp(1.0f, 1.0f / matData.opacity, saturate(matData.opacity * 10.0f));
  light.specularLight *= specularNormalization;
#endif

  float3 litColor = light.diffuseLight + light.specularLight;
  litColor += matData.emissiveColor;

#if RENDER_PASS == RENDER_PASS_FORWARD
#  if defined(USE_FOG) && SHADING_QUALITY == SHADING_QUALITY_NORMAL
#    if BLEND_MODE == BLEND_MODE_ADDITIVE
  matData.opacity *= GetFogAmount(matData.worldPosition);
#    elif BLEND_MODE == BLEND_MODE_MODULATE
  litColor = lerp(1.0, litColor, GetFogAmount(matData.worldPosition));
#    else
  litColor = ApplyFog(litColor, matData.worldPosition);
#    endif
#  endif

  Output.Color = float4(litColor, matData.opacity);

#elif RENDER_PASS == RENDER_PASS_EDITOR
#  if SHADING_QUALITY == SHADING_QUALITY_NORMAL
  if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY)
  {
    Output.Color = float4(SrgbToLinear(light.diffuseLight * Exposure), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY)
  {
    Output.Color = float4(SrgbToLinear(light.specularLight * Exposure), matData.opacity);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT || RenderPass == EDITOR_RENDER_PASS_DECAL_COUNT)
  {
    float lightCount = RenderPass == EDITOR_RENDER_PASS_LIGHT_COUNT ? GET_LIGHT_INDEX(clusterData.counts) : GET_DECAL_INDEX(clusterData.counts);
    float3 heatmap = 0;
    if (lightCount > 0)
    {
      float x = (lightCount - 1) / 16;
      heatmap.r = saturate(x);
      heatmap.g = saturate(2 - x);
    }

    Output.Color = float4(lerp(litColor, heatmap, 0.7), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
  {
#    if defined(USE_TEXCOORD0)
    Output.Color = float4(SrgbToLinear(float3(frac(Input.TexCoord0.xy), 0)), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV1)
  {
#    if defined(USE_TEXCOORD1)
    Output.Color = float4(SrgbToLinear(float3(frac(Input.TexCoord1.xy), 0)), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_COLORS0)
  {
#    if defined(USE_COLOR0)
    Output.Color = float4(SrgbToLinear(Input.Color0.rgb), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_COLORS1)
  {
#    if defined(USE_COLOR1)
    Output.Color = float4(SrgbToLinear(Input.Color1.rgb), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_NORMALS)
  {
#    if defined(USE_NORMAL)
    Output.Color = float4(SrgbToLinear(normalize(Input.Normal) * 0.5 + 0.5), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_VERTEX_TANGENTS)
  {
#    if defined(USE_TANGENT)
    Output.Color = float4(SrgbToLinear(normalize(Input.Tangent) * 0.5 + 0.5), 1);
#    else
    Output.Color = float4(0, 0, 0, 1);
#    endif
  }
  else if (RenderPass == EDITOR_RENDER_PASS_PIXEL_NORMALS)
  {
    Output.Color = float4(SrgbToLinear(matData.worldNormal * 0.5 + 0.5), 1);
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
#    if defined(USE_DEBUG_INTERPOLATOR)
    Output.Color = Input.DebugInterpolator;
#    else
    Output.Color = float4(0.05, 0.05, 0.1, 1);
#    endif
  }
  else
  {
    Output.Color = float4(1.0f, 0.0f, 1.0f, 1.0f);
  }

#  elif SHADING_QUALITY == SHADING_QUALITY_SIMPLIFIED
  Output.Color = float4(litColor, matData.opacity);
#  endif

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
#  error "RENDER_PASS uses undefined value."
#endif

  return Output;
}
