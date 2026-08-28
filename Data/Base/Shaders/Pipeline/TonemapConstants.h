#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

#define TONEMAPMODE_LINEAR 0
#define TONEMAPMODE_REINHARD 1
#define TONEMAPMODE_FILMIC 2
#define TONEMAPMODE_ACES 3
#define TONEMAPMODE_AGX 4

CONSTANT_BUFFER(ezTonemapConstants, 3)
{
  FLOAT4(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Lut1Strength);
  FLOAT1(Lut2Strength);
  FLOAT4(ContrastParams);

  INT1(TonemapMode);
  FLOAT1(WhitePoint);
};
