#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER2(ezParticleSystemConstants, 2, BG_MATERIAL)
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

  // use this instead of world clock for determinism
  FLOAT1(TotalEffectLifeTime);

  // lighting params
  FLOAT1(NormalCurvature);
  FLOAT1(LightDirectionality);

  INT2(ParticlePadding);
};
