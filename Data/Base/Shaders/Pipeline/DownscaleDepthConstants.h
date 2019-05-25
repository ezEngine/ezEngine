#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezDownscaleDepthConstants, 3)
{
  FLOAT2(PixelSize);
  BOOLEAN(LinearizeDepth);
};