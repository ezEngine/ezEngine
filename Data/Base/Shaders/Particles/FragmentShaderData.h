#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezFragmentParticleData)
{
  FLOAT3(Position);
  FLOAT1(dummy1);

  FLOAT3(TangentX);
  FLOAT1(dummy2);

  FLOAT3(TangentZ);
  FLOAT1(dummy3);

  COLOR4UB(Color);
  FLOAT1(Size);
  FLOAT2(dummy4);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezFragmentParticleData> particleData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezFragmentParticleData) == 64);

#endif

