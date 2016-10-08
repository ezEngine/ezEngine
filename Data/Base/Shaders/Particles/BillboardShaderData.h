#pragma once

#include "ParticleSystemConstants.h"

EZ_ALIGN_16(struct) ezBillboardParticleData
{
  FLOAT3(Position);
  FLOAT1(Size);
  COLOR4UB(Color);
  FLOAT3(dummy1);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezBillboardParticleData> particleData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezBillboardParticleData) == 32);

#endif

