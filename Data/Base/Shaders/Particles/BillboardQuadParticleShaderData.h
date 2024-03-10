#pragma once

#include "BaseParticleShaderData.h"
#include "ParticleSystemConstants.h"

struct EZ_SHADER_STRUCT ezBillboardQuadParticleShaderData
{
  FLOAT3(Position);
  PACKEDHALF2(RotationOffset, RotationSpeed, RotationOffsetAndSpeed);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezBillboardQuadParticleShaderData> particleBillboardQuadData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardQuadParticleShaderData) == 16);

#endif
