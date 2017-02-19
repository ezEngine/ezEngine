#pragma once

#define EZ_SSE_20   0x20
#define EZ_SSE_30   0x30
#define EZ_SSE_31   0x31
#define EZ_SSE_41   0x41
#define EZ_SSE_42   0x42
#define EZ_SSE_AVX  0x50
#define EZ_SSE_AVX2 0x51

#define EZ_SSE_LEVEL EZ_SSE_41

#if EZ_SSE_LEVEL >= EZ_SSE_20
#include <emmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_30
#include <pmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_31
#include <tmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_41
#include <smmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_42
#include <nmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#include <immintrin.h>
#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  #define EZ_CHECK_SIMD_ALIGNMENT EZ_CHECK_ALIGNMENT_16
#else
  #define EZ_CHECK_SIMD_ALIGNMENT
#endif

namespace ezInternal
{
  typedef __m128 QuadFloat;
  typedef __m128 QuadBool;
  typedef __m128i QuadInt;
}

