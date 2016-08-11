#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(TonemapConstants, 3)
{
  FLOAT1(Exposure);
  FLOAT3(AutoExposureParams);
  
  COLOR(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT1(Contrast);
};

