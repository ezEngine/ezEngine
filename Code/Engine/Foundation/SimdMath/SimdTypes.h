#pragma once

#include <Foundation/Math/Math.h>

struct ezMathAcc
{
  enum Enum
  {
    FULL,
    BITS_23,
    BITS_12
  };
};

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
# if EZ_SSE_LEVEL < EZ_SSE_AVX
#  include <Foundation/SimdMath/Implementation/SSE/SSETypes_inl.h>
# else
#  include <Foundation/SimdMath/Implementation/AVX/AVXTypes_inl.h>
#endif
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_FPU
#  include <Foundation/SimdMath/Implementation/FPU/FPUTypes_inl.h>
#elif EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_NEON
#  include <Foundation/SimdMath/Implementation/NEON/NEONTypes_inl.h>
#else
#  error "Unknown SIMD implementation."
#endif
