#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezVRCompanionViewConstants, 2)
{
  FLOAT2(TargetSize);
};