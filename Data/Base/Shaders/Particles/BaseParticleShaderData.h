#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezBaseParticleShaderData)
{
  PACKEDCOLOR4H(Color);
  PACKEDHALF2(Life, Size, LifeAndSize); // Life: 1 to 0
  UINT1(Variation); // only lower 8 bit
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBaseParticleShaderData> particleBaseData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBaseParticleShaderData) == 16);

#endif

