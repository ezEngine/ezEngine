#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezVRCompanionViewConstants, 2)
{
  FLOAT2(TargetSize);
};