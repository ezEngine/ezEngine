#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezReflectionFilteredSpecularConstants, 3)
{
  FLOAT4(Forward);
  FLOAT4(Up2);
  UINT1(MipLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
  UINT1(OutputIndex);
};
