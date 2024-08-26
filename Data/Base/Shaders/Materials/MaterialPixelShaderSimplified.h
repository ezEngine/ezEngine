#pragma once

#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME

// No need to do alpha test again if we have a depth prepass
#  if defined(FORWARD_PASS_WRITE_DEPTH) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR)
#    if FORWARD_PASS_WRITE_DEPTH == TRUE
#      define USE_ALPHA_TEST
#    endif
#  else
#    define USE_ALPHA_TEST
#  endif

#  if defined(USE_ALPHA_TEST) && defined(MSAA)
#    if MSAA == TRUE
#      define USE_ALPHA_TEST_SUPER_SAMPLING
#    endif
#  endif

#endif

#include <Shaders/Common/LightingSimplified.h>
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
#  if defined(USE_ALPHA_TEST_SUPER_SAMPLING)
  Output.Coverage = coverage;
#  endif
#endif

  ezMaterialData matData = FillMaterialData();
  uint gameObjectId = GetInstanceData().GameObjectID;

#if SHADING_MODE == SHADING_MODE_LIT
  AccumulatedLight light = CalculateLightingSimplified(matData);
#else
  AccumulatedLight light = InitializeLight(matData.diffuseColor, 0.0f);
#endif

  float3 litColor = light.diffuseLight + light.specularLight;
  litColor += matData.emissiveColor;

#if RENDER_PASS == RENDER_PASS_FORWARD

  Output.Color = float4(litColor, matData.opacity);

#elif RENDER_PASS == RENDER_PASS_EDITOR
  Output.Color = float4(litColor, matData.opacity);
#elif RENDER_PASS == RENDER_PASS_WIREFRAME
  Output.Color = float4(litColor, matData.opacity);
#elif (RENDER_PASS == RENDER_PASS_PICKING || RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME)
  Output.Color = RGBA8ToFloat4(gameObjectId);
#elif RENDER_PASS == RENDER_PASS_DEPTH_ONLY

#else
  Output.Color = float4(litColor, matData.opacity);
#  error "RENDER_PASS uses undefined value."
#endif

  return Output;
}
