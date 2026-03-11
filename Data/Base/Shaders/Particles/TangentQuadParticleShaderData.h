#pragma once

#include "BaseParticleShaderData.h"

struct EZ_SHADER_STRUCT ezTangentQuadParticleShaderData
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezTangentQuadParticleShaderData> particleTangentQuadData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(ezTangentQuadParticleShaderData) == 48);

#endif
