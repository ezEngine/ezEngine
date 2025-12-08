#pragma once

#if EZ_SSE_LEVEL >= EZ_SSE_20
#  include <emmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_30
#  include <pmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_31
#  include <tmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_41
#  include <smmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_42
#  include <nmmintrin.h>
#endif

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#  include <immintrin.h>
#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
#  define EZ_CHECK_SIMD_ALIGNMENT(x) EZ_CHECK_ALIGNMENT(x, 16)
#else
#  define EZ_CHECK_SIMD_ALIGNMENT(x)
#endif

namespace ezInternal
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  using QuadDouble = __m256d;
  using QuadBoolWide = __m256d;

#else
  struct QuadDouble
  {
    __m128d xy;
    __m128d zw;
  };
  struct QuadBoolWide
  {
    __m128d xy;
    __m128d zw;
  };
#endif

  using QuadFloat = __m128;
  using QuadBool = __m128;
  using QuadInt = __m128i;
  using QuadUInt = __m128i;
} // namespace ezInternal

#include <Foundation/SimdMath/SimdSwizzle.h>

#define EZ_SHUFFLE(a0, a1, b2, b3) ((a0) | ((a1) << 2) | ((b2) << 4) | ((b3) << 6))
#define EZ_SHUFFLE_2(a0, b1) ((a0) | ((b1) << 1))


//swizzle to shuffle
#define EZ_TO_SHUFFLE(s) ((((s) >> 12) & 0x03) | (((s) >> 6) & 0x0c) | ((s) & 0x30) | (((s) << 6) & 0xc0))

/// \brief Shuffles doubles in the same manner as _mm_shuffle_ps but for doubles using sse intrinsics on high and low parts.
EZ_ALWAYS_INLINE void EZ_WIDE_SHUFFLE_SSE(__m128d aLow, __m128d aHigh, __m128d bLow, __m128d bHigh, int imm8, __m128d& outLow, __m128d& outHigh)
{
  __m128d sources[2] = {aLow, aHigh};
  int sel0 = imm8 & 3;
  int sel1 = (imm8 >> 2) & 3;
  int sel2 = (imm8 >> 4) & 3;
  int sel3 = (imm8 >> 6) & 3;

  auto get_double = [&](int sel) -> __m128d {
    int source_idx = sel / 2;
    int double_idx = sel % 2;
    __m128d temp0 = _mm_shuffle_pd(sources[source_idx], sources[source_idx], 0);
    __m128d temp1 = _mm_shuffle_pd(sources[source_idx], sources[source_idx], 3);
    __m128d mask = _mm_cmpeq_pd(_mm_set1_pd(double_idx), _mm_set1_pd(0));
    return _mm_or_pd(_mm_andnot_pd(mask, temp1), _mm_and_pd(mask, temp0));
  };

  __m128d d0 = get_double(sel0);
  __m128d d1 = get_double(sel1);
  __m128d d2 = get_double(sel2);
  __m128d d3 = get_double(sel3);

  outLow = _mm_shuffle_pd(d0, d1, 0);
  outHigh = _mm_shuffle_pd(d2, d3, 0);
}

EZ_ALWAYS_INLINE __m256d EZ_WIDE_SHUFFLE_AVX(__m256d a, __m256d b, int imm8)
{

}