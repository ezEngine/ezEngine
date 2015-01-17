#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

CONSTANT_BUFFER(GlobalConstants, 0)
{
  FLOAT1(GameTime);
  COLOR(AmbientColor);
};


