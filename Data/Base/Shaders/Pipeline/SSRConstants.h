#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

#define SSR_CLASSIFY_THREAD_GROUP_SIZE 8
#define SSR_TRACE_THREAD_GROUP_SIZE 64
#define SSR_TEMPORAL_THREAD_GROUP_SIZE 8
#define SSR_HIZ_THREAD_GROUP_SIZE 8
#define SSR_BLUR_THREAD_GROUP_SIZE 8

CONSTANT_BUFFER(ezSSRConstants, 4)
{
  UINT1(SSRResolutionX);
  UINT1(SSRResolutionY);
  FLOAT1(SSRThickness);
  UINT1(SSRMaxSteps);

  FLOAT1(SSRMaxDistance);
  FLOAT1(SSRRoughnessThreshold);
  FLOAT1(SSRTemporalBlendWeight);
  FLOAT1(SSRBRDFBias);

  FLOAT2(SSRInverseResolution);
  UINT1(SSRHiZMipCount);
  UINT1(SSRHiZResolutionX);

  UINT1(SSRHiZResolutionY);
  UINT1(SSRCurrentMipLevel);
  BOOL1(SSRHasGBuffer);
  UINT1(SSRFrameIndex);

  MAT4(SSRPrevWorldToClipMatrix);

  FLOAT1(SSREdgeFadeStart);
  FLOAT1(SSREdgeFadeEnd);
  FLOAT1(SSRBlurRadius);
  FLOAT1(SSRBlurSharpness);
};
