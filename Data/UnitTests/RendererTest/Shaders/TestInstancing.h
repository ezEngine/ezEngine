#pragma once

#include "../../../Base/Shaders/Common/Platforms.h"

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

struct EZ_ALIGN_16(ezTestShaderData)
{
  FLOAT4(InstanceColor);
  TRANSFORM(InstanceTransform);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezTestShaderData> instancingData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezTestShaderData) == 64);

#endif