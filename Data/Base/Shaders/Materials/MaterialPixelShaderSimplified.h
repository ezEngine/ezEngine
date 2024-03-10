#pragma once

#if RENDER_PASS != RENDER_PASS_FORWARD
#  error "MaterialPixelShaderSimplified supports only the forward render pass"
#endif

#if BLEND_MODE == BLEND_MODE_MASKED && RENDER_PASS != RENDER_PASS_WIREFRAME

#  if defined(MSAA)
#    if MSAA == TRUE
#      define USE_ALPHA_TEST_SUPER_SAMPLING
#    endif
#  endif

#endif

#include <Shaders/Common/LightingSimplified.h>
#include <Shaders/Materials/MaterialHelper.h>

struct PS_OUT
{
  float4 Color : SV_Target;

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

  PS_OUT Output;

#if BLEND_MODE == BLEND_MODE_MASKED
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

#if SHADING_MODE == SHADING_MODE_LIT
  AccumulatedLight light = CalculateLightingSimplified(matData);
#else
  AccumulatedLight light = InitializeLight(matData.diffuseColor, 0.0f);
#endif

  float3 litColor = light.diffuseLight + light.specularLight;
  litColor += matData.emissiveColor;

#if RENDER_PASS == RENDER_PASS_FORWARD

  Output.Color = float4(litColor, matData.opacity);

#else
  Output.Color = float4(litColor, matData.opacity);
#  error "RENDER_PASS uses undefined value."
#endif

  return Output;
}
