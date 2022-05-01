#pragma once

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"
#include "../../../Base/Shaders/Common/Platforms.h"

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
