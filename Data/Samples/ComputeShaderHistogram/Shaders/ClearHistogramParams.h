#pragma once

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

BEGIN_PUSH_CONSTANTS(ClearHistogramParams)
{
  UINT1(Width);
  UINT1(Height);
}
END_PUSH_CONSTANTS(ClearHistogramParams)
