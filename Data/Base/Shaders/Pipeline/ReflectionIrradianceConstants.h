#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezReflectionIrradianceConstants, 3)
{
  FLOAT1(LodLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
  UINT1(OutputIndex);
};
