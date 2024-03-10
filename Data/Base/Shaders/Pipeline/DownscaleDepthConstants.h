#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezDownscaleDepthConstants, 3)
{
  FLOAT2(PixelSize);
  BOOL1(LinearizeDepth);
};