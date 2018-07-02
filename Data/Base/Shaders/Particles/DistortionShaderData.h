#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezDistortionParticleData)
{
  TRANSFORM(Transform);
  COLOR4UB(Color);
  FLOAT1(Life); // 1 to 0
  FLOAT1(Size);
  FLOAT1(dummy);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezDistortionParticleData> particleData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezDistortionParticleData) == 64);

#endif

