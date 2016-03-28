#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(BlurConstants, 3)
{
  INT1(BlurRadius);
};

