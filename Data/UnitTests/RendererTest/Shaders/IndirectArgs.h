#pragma once

#include "../../../Base/Shaders/Common/ConstantBufferMacros.h"

BEGIN_PUSH_CONSTANTS(ezIndirectArgs)
{
  UINT1(Arg0);
  UINT1(Arg1);
  UINT1(Arg2);
  UINT1(Arg3);
  UINT1(Arg4);
  UINT1(_Pad0);
  UINT1(_Pad1);
  UINT1(_Pad2);
}
END_PUSH_CONSTANTS(ezIndirectArgs)
