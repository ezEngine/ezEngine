#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

#define LENS_FLARE_INVERSE_TONEMAP (1 << 0)
#define LENS_FLARE_GREYSCALE_TEXTURE (1 << 1)
#define LENS_FLARE_APPLY_FOG (1 << 3)

struct EZ_SHADER_STRUCT ezPerLensFlareData
{
  FLOAT3(WorldSpacePosition);
  FLOAT1(Size);
  FLOAT1(MaxScreenSize);
  FLOAT1(OcclusionRadius);
  FLOAT1(OcclusionSpread);
  FLOAT1(DepthOffset);
  UINT1(AspectRatioAndShift);
  UINT1(Flags);
  UINT1(ColorRG);
  UINT1(ColorBA);
};
