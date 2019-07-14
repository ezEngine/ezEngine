#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezReflectionIrradianceConstants, 3)
{
  FLOAT1(LodLevel);
  FLOAT1(Intensity);
  FLOAT1(Saturation);
  UINT1(OutputIndex);
};

