#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezReflectionFilteredSpecularConstants, 3)
{
  UINT1(MipLevel);
  UINT1(OutputWidth);
  UINT1(OutputHeight);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
  FLOAT1(DUMMY1);
  FLOAT1(DUMMY2);
  FLOAT1(DUMMY3);
};
