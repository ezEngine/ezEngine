#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

CONSTANT_BUFFER(ezKrautTreeConstants, 4)
{
  FLOAT3(LeafCenter);
  FLOAT1(LeafShadowOffset);
};
