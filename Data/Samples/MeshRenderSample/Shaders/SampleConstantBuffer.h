#pragma once

// This file is included both in shader code and in C++ code

CONSTANT_BUFFER(ezMeshRenderSampleConstants, 2)
{
  MAT4(ModelMatrix);
  MAT4(ViewProjectionMatrix);
};
