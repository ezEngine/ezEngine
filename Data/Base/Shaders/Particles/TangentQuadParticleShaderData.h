#pragma once

#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezTangentQuadParticleShaderData)
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

StructuredBuffer<ezTangentQuadParticleShaderData> particleTangentQuadData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezTangentQuadParticleShaderData) == 48);

#endif

