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

StructuredBuffer<ezBillboardQuadParticleShaderData> particleBillboardQuadData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(ezBillboardQuadParticleShaderData) == 16);

#endif
