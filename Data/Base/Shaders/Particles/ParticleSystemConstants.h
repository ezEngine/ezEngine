#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezParticleSystemConstants, 2)
{
	MAT4(ObjectToWorldMatrix);
  FLOAT1(SnapshotFraction);
  INT1(NumUsedTrailPoints);
  UINT1(NumSpritesX);
  UINT1(NumSpritesY);
  FLOAT1(DistortionStrength);
};

