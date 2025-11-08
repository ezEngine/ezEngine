#pragma once

#include <Shaders/Common/GlobalConstants.h>

struct VS_OUT
{
  float4 Position : SV_Position;
  float2 TexCoord0 : TEXCOORD0;
#if CAMERA_MODE == CAMERA_MODE_STEREO
  uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
#endif
};

typedef VS_OUT PS_IN;
