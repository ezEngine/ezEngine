#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezBillboardParticleData)
{
  TRANSFORM(Transform);
  COLOR4UB(Color);
  FLOAT1(Life); // 1 to 0
  FLOAT1(Size);
  FLOAT1(dummy);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardParticleData> particleData;

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardParticleData) == 64);

#endif

