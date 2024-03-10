#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezBloomConstants, 3)
{
  FLOAT2(PixelSize);
  FLOAT1(BloomThreshold);
  FLOAT1(BloomIntensity);

  COLOR4F(TintColor);
};
