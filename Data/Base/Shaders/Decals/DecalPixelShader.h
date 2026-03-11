#pragma once

#include <Shaders/Common/Common.h>
#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Decals/DecalInterpolator.h>

float3 GetBaseColor();
float GetOpacity();

struct PS_GLOBALS
{
  PS_IN Input;
};
static PS_GLOBALS G;

float4 main(PS_IN Input)
  : SV_Target
{
  G.Input = Input;

  float4 output = float4(GetBaseColor(), GetOpacity());

  return output;
}
