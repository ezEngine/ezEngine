// SSE2 implementation of _mmw_* macros for comparison testing.
// This sets up the SSE2 MOC SIMD abstraction and then includes the shared tests.

#include <RendererTest/RendererTestPCH.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

#include <immintrin.h>

#define FORCE_INLINE EZ_ALWAYS_INLINE

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE2 type definitions (matching MaskedOcclusionCulling.cpp SSE2 namespace)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef __m128 __mw;
typedef __m128i __mwi;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE2 _mmw_* macros (from MaskedOcclusionCulling.cpp)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_ps            _mm_set1_ps
#define _mmw_setzero_ps         _mm_setzero_ps
#define _mmw_add_ps(a, b)       _mm_add_ps(a, b)
#define _mmw_sub_ps(a, b)       _mm_sub_ps(a, b)

static FORCE_INLINE __m128 _mmw_and_ps(const __m128& a, const __m128& b) { return _mm_and_ps(a, b); }
static FORCE_INLINE __m128 _mmw_or_ps(const __m128& a, const __m128& b) { return _mm_or_ps(a, b); }
static FORCE_INLINE __m128 _mmw_xor_ps(const __m128& a, const __m128& b) { return _mm_xor_ps(a, b); }
static FORCE_INLINE __m128 _mmw_andnot_ps(const __m128& a, const __m128& b) { return _mm_andnot_ps(a, b); }
static FORCE_INLINE __m128 _mmw_neg_ps(const __m128& a) { return _mm_xor_ps(a, _mm_set1_ps(-0.0f)); }
static FORCE_INLINE __m128 _mmw_abs_ps(const __m128& a) { return _mm_and_ps(a, _mm_castsi128_ps(_mm_set1_epi32(0x7FFFFFFF))); }
static FORCE_INLINE __m128 _mmw_mul_ps(const __m128& a, const __m128& b) { return _mm_mul_ps(a, b); }
static FORCE_INLINE __m128 _mmw_div_ps(const __m128& a, const __m128& b) { return _mm_div_ps(a, b); }
static FORCE_INLINE __m128 _mmw_min_ps(const __m128& a, const __m128& b) { return _mm_min_ps(a, b); }
static FORCE_INLINE __m128 _mmw_max_ps(const __m128& a, const __m128& b) { return _mm_max_ps(a, b); }
static FORCE_INLINE __m128 _mmw_cmpge_ps(const __m128& a, const __m128& b) { return _mm_cmpge_ps(a, b); }
static FORCE_INLINE __m128 _mmw_cmpgt_ps(const __m128& a, const __m128& b) { return _mm_cmpgt_ps(a, b); }
static FORCE_INLINE __m128 _mmw_cmpeq_ps(const __m128& a, const __m128& b) { return _mm_cmpeq_ps(a, b); }
static FORCE_INLINE __m128 _mmw_fmadd_ps(const __m128& a, const __m128& b, const __m128& c) { return _mm_add_ps(_mm_mul_ps(a, b), c); }
static FORCE_INLINE __m128 _mmw_fmsub_ps(const __m128& a, const __m128& b, const __m128& c) { return _mm_sub_ps(_mm_mul_ps(a, b), c); }
static FORCE_INLINE int _mmw_movemask_ps(const __m128& v) { return _mm_movemask_ps(v); }
static FORCE_INLINE __m128 _mmw_cvtepi32_ps(const __m128i& v) { return _mm_cvtepi32_ps(v); }
static FORCE_INLINE __m128i _mmw_cvtps_epi32(const __m128& v) { return _mm_cvtps_epi32(v); }
static FORCE_INLINE __m128i _mmw_cvttps_epi32(const __m128& v) { return _mm_cvttps_epi32(v); }

// SSE2 blendv fallback (same as MOC SSE2 namespace)
static FORCE_INLINE __m128 _mmw_blendv_ps(const __m128& a, const __m128& b, const __m128& c)
{
  __m128 cond = _mm_castsi128_ps(_mm_srai_epi32(_mm_castps_si128(c), 31));
  return _mm_or_ps(_mm_andnot_ps(cond, a), _mm_and_ps(cond, b));
}

static FORCE_INLINE __m128i _mmw_blendv_epi32(const __m128i& a, const __m128i& b, const __m128i& c)
{
  return _mm_castps_si128(_mmw_blendv_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b), _mm_castsi128_ps(c)));
}

// SSE2 floor/ceil fallback
static FORCE_INLINE __m128 _mmw_floor_ps(const __m128& a)
{
  int originalMode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_DOWN);
  __m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
  _MM_SET_ROUNDING_MODE(originalMode);
  return rounded;
}

static FORCE_INLINE __m128 _mmw_ceil_ps(const __m128& a)
{
  int originalMode = _MM_GET_ROUNDING_MODE();
  _MM_SET_ROUNDING_MODE(_MM_ROUND_UP);
  __m128 rounded = _mm_cvtepi32_ps(_mm_cvtps_epi32(a));
  _MM_SET_ROUNDING_MODE(originalMode);
  return rounded;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SSE2 integer operations
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_epi32         _mm_set1_epi32
#define _mmw_setzero_epi32      _mm_setzero_si128

static FORCE_INLINE __m128i _mmw_and_epi32(const __m128i& a, const __m128i& b) { return _mm_and_si128(a, b); }
static FORCE_INLINE __m128i _mmw_or_epi32(const __m128i& a, const __m128i& b) { return _mm_or_si128(a, b); }
static FORCE_INLINE __m128i _mmw_xor_epi32(const __m128i& a, const __m128i& b) { return _mm_xor_si128(a, b); }
static FORCE_INLINE __m128i _mmw_not_epi32(const __m128i& a) { return _mm_xor_si128(a, _mm_set1_epi32(~0)); }
static FORCE_INLINE __m128i _mmw_andnot_epi32(const __m128i& a, const __m128i& b) { return _mm_andnot_si128(a, b); }
static FORCE_INLINE __m128i _mmw_neg_epi32(const __m128i& a) { return _mm_sub_epi32(_mm_set1_epi32(0), a); }
static FORCE_INLINE __m128i _mmw_add_epi32(const __m128i& a, const __m128i& b) { return _mm_add_epi32(a, b); }
static FORCE_INLINE __m128i _mmw_sub_epi32(const __m128i& a, const __m128i& b) { return _mm_sub_epi32(a, b); }
static FORCE_INLINE __m128i _mmw_srai_epi32(const __m128i& v, int imm) { return _mm_srai_epi32(v, imm); }
static FORCE_INLINE __m128i _mmw_srli_epi32(const __m128i& v, int imm) { return _mm_srli_epi32(v, imm); }
static FORCE_INLINE __m128i _mmw_slli_epi32(const __m128i& v, int imm) { return _mm_slli_epi32(v, imm); }
static FORCE_INLINE __m128i _mmw_cmpeq_epi32(const __m128i& a, const __m128i& b) { return _mm_cmpeq_epi32(a, b); }
static FORCE_INLINE __m128i _mmw_cmpgt_epi32(const __m128i& a, const __m128i& b) { return _mm_cmpgt_epi32(a, b); }
static FORCE_INLINE __m128i _mmw_subs_epu16(const __m128i& a, const __m128i& b) { return _mm_subs_epu16(a, b); }

// SSE2 fallbacks for SSE4.1 ops
static FORCE_INLINE __m128i _mmw_mullo_epi32(const __m128i& a, const __m128i& b)
{
  __m128i even = _mm_and_si128(_mm_mul_epu32(a, b), _mm_setr_epi32(~0, 0, ~0, 0));
  __m128i odd = _mm_slli_epi64(_mm_mul_epu32(_mm_srli_epi64(a, 32), _mm_srli_epi64(b, 32)), 32);
  return _mm_or_si128(even, odd);
}

static FORCE_INLINE __m128i _mmw_min_epi32(const __m128i& a, const __m128i& b)
{
  __m128i cond = _mm_cmpgt_epi32(a, b);
  return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
}

static FORCE_INLINE __m128i _mmw_max_epi32(const __m128i& a, const __m128i& b)
{
  __m128i cond = _mm_cmpgt_epi32(b, a);
  return _mm_or_si128(_mm_andnot_si128(cond, a), _mm_and_si128(cond, b));
}

static FORCE_INLINE __m128i _mmw_abs_epi32(const __m128i& a)
{
  __m128i mask = _mm_cmplt_epi32(a, _mm_setzero_si128());
  return _mm_add_epi32(_mm_xor_si128(a, mask), _mm_srli_epi32(mask, 31));
}

static FORCE_INLINE int _mmw_testz_epi32(const __m128i& a, const __m128i& b)
{
  return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(a, b), _mm_setzero_si128())) == 0xFFFF;
}

// SSE2 transpose fallback
static FORCE_INLINE __m128i _mmw_transpose_epi8(const __m128i& a)
{
  __m128i res = a;
  const __m128i mask = _mm_setr_epi8(~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0, ~0, 0);
  res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
  res = _mm_packus_epi16(_mm_and_si128(res, mask), _mm_srli_epi16(res, 8));
  return res;
}

// SSE2 sllv_ones fallback
#define MAKE_ACCESSOR(name, simd_type, base_type, is_const, elements) \
  FORCE_INLINE is_const base_type* name(is_const simd_type& a)       \
  {                                                                   \
    union accessor                                                    \
    {                                                                 \
      simd_type m_native;                                             \
      base_type m_array[elements];                                    \
    };                                                                \
    is_const accessor* acs = reinterpret_cast<is_const accessor*>(&a);\
    return acs->m_array;                                              \
  }

MAKE_ACCESSOR(simd_f32, __m128, float, , 4)
MAKE_ACCESSOR(simd_f32, __m128, float, const, 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, , 4)
MAKE_ACCESSOR(simd_i32, __m128i, int, const, 4)

static FORCE_INLINE __m128i _mmw_sllv_ones(const __m128i& ishift)
{
  __m128i shift = _mmw_min_epi32(ishift, _mm_set1_epi32(32));

  static const unsigned int maskLUT[33] = {
    ~0U << 0, ~0U << 1, ~0U << 2, ~0U << 3, ~0U << 4, ~0U << 5, ~0U << 6, ~0U << 7,
    ~0U << 8, ~0U << 9, ~0U << 10, ~0U << 11, ~0U << 12, ~0U << 13, ~0U << 14, ~0U << 15,
    ~0U << 16, ~0U << 17, ~0U << 18, ~0U << 19, ~0U << 20, ~0U << 21, ~0U << 22, ~0U << 23,
    ~0U << 24, ~0U << 25, ~0U << 26, ~0U << 27, ~0U << 28, ~0U << 29, ~0U << 30, ~0U << 31,
    0U};

  __m128i retMask;
  simd_i32(retMask)[0] = (int)maskLUT[simd_i32(shift)[0]];
  simd_i32(retMask)[1] = (int)maskLUT[simd_i32(shift)[1]];
  simd_i32(retMask)[2] = (int)maskLUT[simd_i32(shift)[2]];
  simd_i32(retMask)[3] = (int)maskLUT[simd_i32(shift)[3]];
  return retMask;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Y>
static FORCE_INLINE T simd_cast(Y A);
template <>
FORCE_INLINE __m128 simd_cast<__m128>(float A) { return _mm_set1_ps(A); }
template <>
FORCE_INLINE __m128 simd_cast<__m128>(__m128i A) { return _mm_castsi128_ps(A); }
template <>
FORCE_INLINE __m128 simd_cast<__m128>(__m128 A) { return A; }
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(int A) { return _mm_set1_epi32(A); }
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(__m128 A) { return _mm_castps_si128(A); }
template <>
FORCE_INLINE __m128i simd_cast<__m128i>(__m128i A) { return A; }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test registration
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EZ_CREATE_SIMPLE_RENDERER_TEST_GROUP(MOCSIMDTest)

#define MOC_TEST_PREFIX SSE

#include "MOCSIMDTests.inl"
