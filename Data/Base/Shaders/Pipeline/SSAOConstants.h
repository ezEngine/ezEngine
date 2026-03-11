#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER2(ezSSAOConstants, 3, BG_RENDER_PASS)
{
  FLOAT2(TexCoordsScale);
  FLOAT2(FadeOutParams);

  FLOAT1(WorldRadius);
  FLOAT1(MaxScreenSpaceRadius);
  FLOAT1(Contrast);
  FLOAT1(Intensity);

  FLOAT1(PositionBias);
  FLOAT1(MipLevelScale);
  FLOAT1(DepthBlurScale);
  FLOAT1(FadeOutEnd);
};
