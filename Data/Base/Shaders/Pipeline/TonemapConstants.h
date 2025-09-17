#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezTonemapConstants, 3)
{
  FLOAT4(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Lut1Strength);
  FLOAT1(Lut2Strength);
  FLOAT4(ContrastParams);
  FLOAT1(WhitePoint);
};
