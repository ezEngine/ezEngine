#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER2(ezLightShaftsConstants, 3, BG_RENDER_PASS)
{
  FLOAT1(LightShaftsIntensity);
  FLOAT1(LightShaftsMaxBrightness);
  FLOAT1(LightShaftsBrightnessThreshold);
  FLOAT1(LightShaftsDiskMaskRadius);

  FLOAT2(LightShaftsOriginUVs);
  UINT1(LightShaftsNumBlurSamples);
  FLOAT1(LightShaftsBlurStep);
};
