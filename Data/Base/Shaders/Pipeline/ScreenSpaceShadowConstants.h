#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

#define SSS_THREAD_GROUP_SIZE 8

CONSTANT_BUFFER(ezScreenSpaceShadowConstants, 4)
{
  FLOAT3(SSSLightDirectionWS);
  FLOAT1(SSSMaxRayDistance);

  UINT1(SSSMaxSteps);
  FLOAT1(SSSSurfaceThickness);
  FLOAT1(SSSShadowIntensity);
  UINT1(SSSFrameIndex);

  FLOAT2(SSSTexelSize);
  UINT1(SSSResolutionX);
  UINT1(SSSResolutionY);
};
