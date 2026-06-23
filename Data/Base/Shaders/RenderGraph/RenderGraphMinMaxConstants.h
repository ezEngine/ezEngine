#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

BEGIN_PUSH_CONSTANTS(ezRenderGraphMinMaxConstants)
{
  UINT2(TextureSize);
  INT1(SampleIndex);
  UINT1(SampleCount);
  UINT1(ChannelMask);
}
END_PUSH_CONSTANTS(ezRenderGraphMinMaxConstants)
