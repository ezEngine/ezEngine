#pragma once

#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezQuadParticleShaderData)
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezQuadParticleShaderData> particleQuadData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezQuadParticleShaderData) == 48);

#endif

