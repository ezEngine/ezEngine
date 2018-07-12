#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezParticleSystemConstants, 2)
{
	MAT4(ObjectToWorldMatrix);

  // random variations, can be combined with flipbook animations
  UINT1(TextureAtlasVariationFramesX);
  UINT1(TextureAtlasVariationFramesY);

  // flip-book animations
  UINT1(TextureAtlasFlipbookFramesX);
  UINT1(TextureAtlasFlipbookFramesY);

  // for trail particles
  FLOAT1(SnapshotFraction);
  INT1(NumUsedTrailPoints);

  // heat haze distortion (pixel offset in screen-space)
  FLOAT1(DistortionStrength);
};

