#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

struct EZ_SHADER_STRUCT ezTestShaderData
{
  FLOAT4(InstanceColor);
  TRANSFORM(InstanceTransform);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezTestShaderData> instancingData;

#else // C++

static_assert(sizeof(ezTestShaderData) == 64);

#endif
