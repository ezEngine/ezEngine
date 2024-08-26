#pragma once

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

BEGIN_PUSH_CONSTANTS(ezTestData)
{
  FLOAT4(VertexColor);
  FLOAT4(Vertex0);
  FLOAT4(Vertex1);
  FLOAT4(Vertex2);
}
END_PUSH_CONSTANTS(ezTestData)
