#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezBilateralBlurConstants, 3)
{
  UINT1(BlurRadius);
  FLOAT1(GaussianFalloff);  // 1 / (2 * sigma * sigma)
  FLOAT1(Sharpness); // 0 is aquivalent with a Gaussian blur
};

