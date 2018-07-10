#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezBaseParticleShaderData)
{
  COLOR4F(Color);
  FLOAT1(Life); // 1 to 0
  FLOAT1(Size);
  INT1(NumPoints); // used by trail particles
  FLOAT1(dummy1);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBaseParticleShaderData> particleBaseData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBaseParticleShaderData) == 32);

#endif

