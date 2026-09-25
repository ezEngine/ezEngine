#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezUpscaleConstants, 3)
{
  FLOAT2(InputTexelSize);
  FLOAT1(Sharpness);
};
