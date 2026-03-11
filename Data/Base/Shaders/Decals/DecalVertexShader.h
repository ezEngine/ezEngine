#pragma once

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Decals/DecalInterpolator.h>

VS_OUT FillVertexData(VS_IN Input)
{
  float3 objectPosition = Input.Position;

  VS_OUT Output;
  Output.Position = float4(objectPosition, 1.0);

  Output.TexCoord0 = objectPosition.xy * float2(0.5, -0.5) + 0.5;

  return Output;
}
