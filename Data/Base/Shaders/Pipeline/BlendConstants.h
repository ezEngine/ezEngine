#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezBlendConstants, 3)
{
  FLOAT1(BlendFactor);
  FLOAT1(BlendFactor2);
  FLOAT1(BlendFactor3);
  FLOAT1(BlendFactor4);
};
