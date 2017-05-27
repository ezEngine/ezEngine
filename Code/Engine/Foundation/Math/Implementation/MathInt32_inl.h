#pragma once

namespace ezMath
{
  template<>
  EZ_ALWAYS_INLINE ezInt32 BasicType<ezInt32>::MaxValue()
  {
    return 2147483647;
  }

  template<>
  EZ_ALWAYS_INLINE ezUInt32 BasicType<ezUInt32>::MaxValue()
  {
    return 4294967295;
  }
}

