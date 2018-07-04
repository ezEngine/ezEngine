#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezPointParticleData)
{
  FLOAT3(Position);
  COLOR4UB(Color);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezPointParticleData> particleData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezPointParticleData) == 16);

#endif

