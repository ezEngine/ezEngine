#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezTrailParticleData)
{
  COLOR4UB(Color);
  FLOAT1(Life); // 1 to 0
  FLOAT1(Size);
  INT1(NumPoints);
};

struct EZ_ALIGN_16(ezTrailParticlePointsData8)
{
  FLOAT4(Positions[8]);
};

struct EZ_ALIGN_16(ezTrailParticlePointsData16)
{
  FLOAT4(Positions[16]);
};

struct EZ_ALIGN_16(ezTrailParticlePointsData32)
{
  FLOAT4(Positions[32]);
};

struct EZ_ALIGN_16(ezTrailParticlePointsData64)
{
  FLOAT4(Positions[64]);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezTrailParticleData> particleData;

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT8
  StructuredBuffer<ezTrailParticlePointsData8> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT16
  StructuredBuffer<ezTrailParticlePointsData16> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT32
  StructuredBuffer<ezTrailParticlePointsData32> particlePointsData;
#endif

#if PARTICLE_TRAIL_POINTS == PARTICLE_TRAIL_POINTS_COUNT64
  StructuredBuffer<ezTrailParticlePointsData64> particlePointsData;
#endif

#else // C++

EZ_CHECK_AT_COMPILETIME(sizeof(ezTrailParticleData) == 16);

#endif

