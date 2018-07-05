#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezBillboardQuadParticleShaderData)
{
  TRANSFORM(Transform);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardQuadParticleShaderData) == 48);

#endif

