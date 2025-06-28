#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

CONSTANT_BUFFER2(ezKrautTreeConstants, 4, BG_RENDER_PASS)
{
  FLOAT3(LeafCenter);
  FLOAT1(LeafShadowOffset);
};
