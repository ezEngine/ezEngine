#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezCopyConstants, 3)
{
  INT2(Offset);
};
