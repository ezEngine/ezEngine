#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezBillboardQuadParticleShaderData)
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardQuadParticleShaderData) == 16);

#endif

