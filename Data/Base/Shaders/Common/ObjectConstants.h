#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

CONSTANT_BUFFER(ObjectConstants, 2)
{
  MAT4(ObjectToWorldMatrix);
  MAT4(ObjectToCameraMatrix);
  MAT4(ObjectToScreenMatrix);
  INT1(GameObjectID);
  INT1(PartIndex);
};

