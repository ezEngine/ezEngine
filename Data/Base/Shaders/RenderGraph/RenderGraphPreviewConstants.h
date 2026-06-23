#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

BEGIN_PUSH_CONSTANTS(ezRenderGraphPreviewConstants)
{
  FLOAT4(UVTransform);
  FLOAT2(ValueRange);
  UINT1(ChannelMask);
  INT1(SampleIndex);
  INT2(PixelPosition);
  INT1(HighlightPixel);
}
END_PUSH_CONSTANTS(ezRenderGraphPreviewConstants)
