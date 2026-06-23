#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>

BEGIN_PUSH_CONSTANTS(ezRenderGraphReadbackPixelConstants)
{
  INT2(PixelPosition);
  INT1(SampleIndex);
  UINT1(SampleCount);
  UINT2(TextureSize);
}
END_PUSH_CONSTANTS(ezRenderGraphReadbackPixelConstants)
