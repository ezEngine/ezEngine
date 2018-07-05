#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezBillboardParticleData)
{
  TRANSFORM(Transform);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardParticleData> particleBillboardData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardParticleData) == 48);

#endif

