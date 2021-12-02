#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezReflectionFilteredSpecularConstants, 3)
{
  UINT1(MipLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
};
