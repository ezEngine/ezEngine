#pragma once

#if SHADING_QUALITY != SHADING_QUALITY_SIMPLIFIED
#  error "Functions in LightDataSimplified.h are only for SIMPLIFIED shading quality. Include LightData.h instead."
#endif

#include "ConstantBufferMacros.h"
#include "Platforms.h"

CONSTANT_BUFFER(ezSimplifiedDataConstants, 3)
{
  UINT1(SkyIrradianceIndex);
};
