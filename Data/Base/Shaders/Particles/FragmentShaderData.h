#pragma once

#include "ParticleSystemConstants.h"

EZ_ALIGN_16(struct) ezFragmentParticleData
{
  TRANSFORM(Transform);
  COLOR4UB(Color);
  FLOAT1(Size);
  FLOAT2(dummy1);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezFragmentParticleData> particleData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezFragmentParticleData) == 64);

#endif

