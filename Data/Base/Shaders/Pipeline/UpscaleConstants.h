#pragma once

#include "../Common/ConstantBufferMacros.h"

BEGIN_PUSH_CONSTANTS(ezUpscaleConstants)
{
  FLOAT2(InputTexelSize);
  FLOAT1(Sharpness);
}
END_PUSH_CONSTANTS(ezUpscaleConstants)
