#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezCopyConstants, 3)
{
  INT2(Offset);
};

