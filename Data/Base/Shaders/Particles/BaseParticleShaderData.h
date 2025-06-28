#pragma once

#include "ParticleSystemConstants.h"

struct EZ_SHADER_STRUCT ezBaseParticleShaderData
{
  PACKEDCOLOR4H(Color);
  PACKEDHALF2(Life, Size, LifeAndSize); // Life: 1 to 0
  UINT1(Variation);                     // only lower 8 bit
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezBaseParticleShaderData> particleBaseData BIND_GROUP(BG_DRAW_CALL);

#else // C++

static_assert(sizeof(ezBaseParticleShaderData) == 16);

#endif
