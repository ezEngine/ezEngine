#pragma once

#include "ParticleSystemConstants.h"
#include "BaseParticleShaderData.h"

struct EZ_ALIGN_16(ezTrailParticleShaderData)
{
  INT1(NumPoints);
  FLOAT3(dummy);
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
#if EZ_ENABLED(PLATFORM_SHADER)

StructuredBuffer<ezTrailParticleShaderData> particleTrailData;

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

#endif

