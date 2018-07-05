#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezBaseParticleShaderData)
{
  COLOR4UB(Color);
  FLOAT1(Life); // 1 to 0
  FLOAT1(Size);
  INT1(NumPoints); // used by trail particles
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBaseParticleShaderData> particleBaseData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBaseParticleShaderData) == 16);

#endif

