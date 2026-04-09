#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezScreenSpaceShadowConstants, 4)
{
  FLOAT4(LightCoordinate);
  INT2(WaveOffset);
  FLOAT2(InvDepthTextureSize);

  FLOAT1(SurfaceThickness);
  FLOAT1(ShadowContrast);

  UINT1(EyeIndex);
};

#define WAVE_SIZE 64
#define SAMPLE_COUNT 60
#define HARD_SHADOW_SAMPLES 4
#define FADE_OUT_SAMPLES 24

#define USE_HALF_PIXEL_OFFSET 1
#define USE_UV_PIXEL_BIAS 1
