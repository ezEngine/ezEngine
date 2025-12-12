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
  int sel0 = imm8 & 3;
  int sel1 = (imm8 >> 2) & 3;
  int sel2 = (imm8 >> 4) & 3;
  int sel3 = (imm8 >> 6) & 3;

  __m128d a_sources[2] = {aLow, aHigh};
  __m128d b_sources[2] = {bLow, bHigh};

  auto get_double = [&](int sel, __m128d (&sources)[2]) -> __m128d {
    int source_idx = sel >> 1;
    int double_idx = sel & 1;
    __m128d src = sources[source_idx];
    __m128d low_bcast = _mm_shuffle_pd(src, src, 0);
    __m128d high_bcast = _mm_shuffle_pd(src, src, 3);
    __m128d mask = _mm_cmpeq_pd(_mm_set1_pd(static_cast<double>(double_idx)), _mm_set1_pd(0.0));
    return _mm_or_pd(_mm_andnot_pd(mask, high_bcast), _mm_and_pd(mask, low_bcast));
  };

  __m128d d0 = get_double(sel0, a_sources);
  __m128d d1 = get_double(sel1, a_sources);
  __m128d d2 = get_double(sel2, b_sources);
  __m128d d3 = get_double(sel3, b_sources);

  outLow = _mm_unpacklo_pd(d0, d1);
  outHigh = _mm_unpacklo_pd(d2, d3);
}

#if EZ_SSE_LEVEL >= EZ_SSE_AVX

/// @brief directly swizzle a 4 double __m256d to the output. AVX1
EZ_ALWAYS_INLINE void EZ_WIDE_SWIZZLE_AVX1(__m256d a, ezSwizzle::Enum swizzle, __m256d& out)
{
  /// \todo use direct intrinsics. - remove EZ_WIDE_SHUFFLE_SSE
  __m128d a_lo = _mm256_castpd256_pd128(a);
  __m128d a_hi = _mm256_extractf128_pd(a, 1);
  __m128d out_lo;
  __m128d out_hi;
  EZ_WIDE_SHUFFLE_SSE(a_lo, a_hi, a_lo, a_hi, EZ_TO_SHUFFLE(swizzle), out_lo, out_hi);
  out = _mm256_castpd128_pd256(out_lo);
  out = _mm256_insertf128_pd(out, out_hi, 1);
}


/// \brief Shuffles doubles in the same manner as _mm_shuffle_ps but for doubles using avx1 intrinsics on high and low parts.
EZ_ALWAYS_INLINE void EZ_WIDE_SHUFFLE_AVX1(__m256d a, __m256d b, int imm8, __m256d& out)
{
  /// \todo use direct intrinsics. - remove EZ_WIDE_SHUFFLE_SSE
  __m128d a_lo = _mm256_castpd256_pd128(a);
  __m128d a_hi = _mm256_extractf128_pd(a, 1);
  __m128d b_lo = _mm256_castpd256_pd128(b);
  __m128d b_hi = _mm256_extractf128_pd(b, 1);
  __m128d out_lo;
  __m128d out_hi;
  EZ_WIDE_SHUFFLE_SSE(a_lo, a_hi, b_lo, b_hi, imm8, out_lo, out_hi);
  out = _mm256_castpd128_pd256(out_lo);
  out = _mm256_insertf128_pd(out, out_hi, 1);
}


#define _MM_TRANSPOSE4_PD_AVX(row0, row1, row2, row3) \
do { \
  __m256d tmp0 = _mm256_unpacklo_pd((row0), (row1)); \
  __m256d tmp1 = _mm256_unpackhi_pd((row0), (row1)); \
  __m256d tmp2 = _mm256_unpacklo_pd((row2), (row3)); \
  __m256d tmp3 = _mm256_unpackhi_pd((row2), (row3)); \
  (row0) = _mm256_permute2f128_pd(tmp0, tmp2, 0x20); \
  (row2) = _mm256_permute2f128_pd(tmp0, tmp2, 0x31); \
  (row1) = _mm256_permute2f128_pd(tmp1, tmp3, 0x20); \
  (row3) = _mm256_permute2f128_pd(tmp1, tmp3, 0x31); \
} while (0)

#endif

#define _MM_TRANSPOSE4_PD(row0lo, row0hi, row1lo, row1hi, row2lo, row2hi, row3lo, row3hi) \
do { \
  __m128d tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7; \
  \
  /* Gather x and y components from lo registers */ \
  tmp0 = _mm_unpacklo_pd((row0lo), (row1lo)); /* [x0, x1] */ \
  tmp1 = _mm_unpackhi_pd((row0lo), (row1lo)); /* [y0, y1] */ \
  tmp2 = _mm_unpacklo_pd((row2lo), (row3lo)); /* [x2, x3] */ \
  tmp3 = _mm_unpackhi_pd((row2lo), (row3lo)); /* [y2, y3] */ \
  \
  /* Gather z and w components from hi registers */ \
  tmp4 = _mm_unpacklo_pd((row0hi), (row1hi)); /* [z0, z1] */ \
  tmp5 = _mm_unpackhi_pd((row0hi), (row1hi)); /* [w0, w1] */ \
  tmp6 = _mm_unpacklo_pd((row2hi), (row3hi)); /* [z2, z3] */ \
  tmp7 = _mm_unpackhi_pd((row2hi), (row3hi)); /* [w2, w3] */ \
  \
  /* Assign results */ \
  (row0lo) = tmp0; /* x components [x0, x1] */ \
  (row0hi) = tmp2; /* x components [x2, x3] */ \
  (row1lo) = tmp1; /* y components [y0, y1] */ \
  (row1hi) = tmp3; /* y components [y2, y3] */ \
  (row2lo) = tmp4; /* z components [z0, z1] */ \
  (row2hi) = tmp6; /* z components [z2, z3] */ \
  (row3lo) = tmp5; /* w components [w0, w1] */ \
  (row3hi) = tmp7; /* w components [w2, w3] */ \
} while (0)


