#pragma once

#include "ParticleSystemConstants.h"

struct EZ_ALIGN_16(ezTrailParticleData)
{
  COLOR4UB(Color);
  FLOAT1(Size);
  INT1(NumSegments);
  FLOAT1(dummy1);
};

struct EZ_ALIGN_16(ezTrailParticlePos8Data)
{
  FLOAT3(Positions[8]);
};

struct EZ_ALIGN_16(ezTrailParticlePos16Data)
{
  FLOAT3(Positions[16]);
};

struct EZ_ALIGN_16(ezTrailParticlePos32Data)
{
  FLOAT3(Positions[32]);
};

struct EZ_ALIGN_16(ezTrailParticlePos64Data)
{
  FLOAT3(Positions[64]);
};

// this is only defined during shader compilation
#if EZ_ENABLED(PLATFORM_DX11)

StructuredBuffer<ezTrailParticleData> particleData;

/// \todo Permutations
StructuredBuffer<ezTrailParticlePos64Data> particleSegmentData;

#else

EZ_CHECK_AT_COMPILETIME(sizeof(ezTrailParticleData) == 16);

#endif

