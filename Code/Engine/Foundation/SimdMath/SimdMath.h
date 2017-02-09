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

#define EZ_SIMD_IMPLEMENTATION_SSE 1
#define EZ_SIMD_IMPLEMENTATION_FPU 2

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_SSE
#else
  #define EZ_SIMD_IMPLEMENTATION EZ_SIMD_IMPLEMENTATION_FPU
#endif

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  #include <Foundation/SimdMath/Implementation/SSE/SSEMath_inl.h>
#else
  #include <Foundation/SimdMath/Implementation/FPU/FPUMath_inl.h>
#endif
