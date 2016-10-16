#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezBillboardParticleData)
{
  TRANSFORM(Transform);
  COLOR4UB(Color);
  FLOAT1(Size);
  FLOAT2(dummy1);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardParticleData> particleData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardParticleData) == 64);

#endif

