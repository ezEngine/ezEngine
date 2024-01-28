#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezTestPerFrame, 0)
{
  FLOAT1(Time);
  FLOAT1(Unused1);
  FLOAT1(Unused2);
  FLOAT1(Unused3);
};

CONSTANT_BUFFER(ezTestColors, 2)
{
  FLOAT4(VertexColor);
};

CONSTANT_BUFFER(ezTestPositions, 3)
{
  FLOAT4(Vertex0);
  FLOAT4(Vertex1);
  FLOAT4(Vertex2);
};
