#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

BEGIN_PUSH_CONSTANTS(ezRenderGraphHistogramConstants)
{
  UINT2(TextureSize);
  INT1(SampleIndex);
  UINT1(SampleCount);
  UINT1(ChannelMask);
  FLOAT1(RangeMin);
  FLOAT1(RangeMax);
}
END_PUSH_CONSTANTS(ezRenderGraphHistogramConstants)
