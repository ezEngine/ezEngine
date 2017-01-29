#pragma once

#include "ParticleSystemConstants.h"

#define TRAIL_POINTS 8
#define TRAIL_SEGMENTS (TRAIL_POINTS - 1)

struct EZ_ALIGN_16(ezTrailParticleData)
{
  COLOR4UB(Color);
  FLOAT1(Size);
  INT1(NumPoints);
  FLOAT1(dummy1);
};

struct EZ_ALIGN_16(ezTrailParticlePointsData)
{
  FLOAT4(Positions[TRAIL_POINTS]);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezTrailParticleData> particleData;

/// \todo Permutations
StructuredBuffer<ezTrailParticlePointsData> particlePointsData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezTrailParticleData) == 16);

#endif

