#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezParticleSystemConstants, 2)
{
	MAT4(ObjectToWorldMatrix);

  // flip-book animations
  UINT1(NumSpritesX);
  UINT1(NumSpritesY);

  // for trail particles
  FLOAT1(SnapshotFraction);
  INT1(NumUsedTrailPoints);

  // heat haze distortion (pixel offset in screen-space)
  FLOAT1(DistortionStrength);
};

