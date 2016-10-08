#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezTonemapConstants, 3)
{
  FLOAT1(Exposure);
  FLOAT3(AutoExposureParams);

  COLOR4F(MoodColor);
  FLOAT1(MoodStrength);
  FLOAT1(Saturation);
  FLOAT2(Padding);
  FLOAT4(ContrastParams);
};

