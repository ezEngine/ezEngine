#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezTonemapConstants, 3)
{
  FLOAT4(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Lut1Strength);
  FLOAT1(Lut2Strength);
  FLOAT4(ContrastParams);
};

