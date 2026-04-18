// ezEngine SIMD implementation of _mmw_* macros for comparison testing.
// This sets up the ez SIMD abstraction and then includes the shared tests.

#include <RendererTest/RendererTestPCH.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

#include <Foundation/SimdMath/SimdVec4b.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/SimdMath/SimdVec4u.h>

#define FORCE_INLINE EZ_ALWAYS_INLINE

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef ezSimdVec4f __mw;
typedef ezSimdVec4i __mwi;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Float operations (matching GenericMOC implementation)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_ps(x) ezSimdVec4f(x)
#define _mmw_setzero_ps() ezSimdVec4f::MakeZero()
#define _mmw_add_ps(a, b) ((a) + (b))
#define _mmw_sub_ps(a, b) ((a) - (b))

static FORCE_INLINE ezSimdVec4f _mmw_and_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return (a.ReinterpretAsInt() & b.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_or_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return (a.ReinterpretAsInt() | b.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_xor_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return (a.ReinterpretAsInt() ^ b.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_andnot_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return (~a.ReinterpretAsInt() & b.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_neg_ps(const ezSimdVec4f& a) { return -a; }
static FORCE_INLINE ezSimdVec4f _mmw_abs_ps(const ezSimdVec4f& a) { return a.Abs(); }
static FORCE_INLINE ezSimdVec4f _mmw_mul_ps(const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMul(b); }
static FORCE_INLINE ezSimdVec4f _mmw_div_ps(const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompDiv(b); }
static FORCE_INLINE ezSimdVec4f _mmw_min_ps(const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMin(b); }
static FORCE_INLINE ezSimdVec4f _mmw_max_ps(const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMax(b); }

static FORCE_INLINE ezSimdVec4f _mmw_cmpge_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return ezSimdVec4i::Select(a >= b, ezSimdVec4i(~0), ezSimdVec4i(0)).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_cmpgt_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return ezSimdVec4i::Select(a > b, ezSimdVec4i(~0), ezSimdVec4i(0)).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_cmpeq_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return ezSimdVec4i::Select(a == b, ezSimdVec4i(~0), ezSimdVec4i(0)).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_fmadd_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return ezSimdVec4f::MulAdd(a, b, c);
}

static FORCE_INLINE ezSimdVec4f _mmw_fmsub_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return ezSimdVec4f::MulSub(a, b, c);
}

static FORCE_INLINE ezSimdVec4f _mmw_blendv_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& mask)
{
  ezSimdVec4b cmp = mask.ReinterpretAsInt() < ezSimdVec4i(0);
  return ezSimdVec4f::Select(cmp, b, a);
}

static FORCE_INLINE int _mmw_movemask_ps(const ezSimdVec4f& v)
{
  ezSimdVec4b negative = v.ReinterpretAsInt() < ezSimdVec4i(0);
  return (int)negative.GetMoveMask();
}

static FORCE_INLINE ezSimdVec4f _mmw_floor_ps(const ezSimdVec4f& a) { return a.Floor(); }
static FORCE_INLINE ezSimdVec4f _mmw_ceil_ps(const ezSimdVec4f& a) { return a.Ceil(); }

#define _mmw_cvtepi32_ps(v) ((v).ToFloat())

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Integer operations
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_epi32(x) ezSimdVec4i(x)
#define _mmw_setzero_epi32() ezSimdVec4i(0)

static FORCE_INLINE ezSimdVec4i _mmw_and_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a & b; }
static FORCE_INLINE ezSimdVec4i _mmw_or_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a | b; }
static FORCE_INLINE ezSimdVec4i _mmw_xor_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a ^ b; }
static FORCE_INLINE ezSimdVec4i _mmw_not_epi32(const ezSimdVec4i& a) { return ~a; }
static FORCE_INLINE ezSimdVec4i _mmw_andnot_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return ~a & b; }
static FORCE_INLINE ezSimdVec4i _mmw_neg_epi32(const ezSimdVec4i& a) { return -a; }
static FORCE_INLINE ezSimdVec4i _mmw_add_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a + b; }
static FORCE_INLINE ezSimdVec4i _mmw_sub_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a - b; }
static FORCE_INLINE ezSimdVec4i _mmw_srai_epi32(const ezSimdVec4i& v, int imm) { return v >> (ezUInt32)imm; }

static FORCE_INLINE ezSimdVec4i _mmw_srli_epi32(const ezSimdVec4i& v, int imm)
{
  ezSimdVec4u u;
  u.m_v = (ezInternal::QuadUInt)v.m_v;
  u = u >> (ezUInt32)imm;
  ezSimdVec4i r;
  r.m_v = (ezInternal::QuadInt)u.m_v;
  return r;
}

static FORCE_INLINE ezSimdVec4i _mmw_slli_epi32(const ezSimdVec4i& v, int imm) { return v << (ezUInt32)imm; }

static FORCE_INLINE ezSimdVec4i _mmw_cmpeq_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ezSimdVec4i::Select(a == b, ezSimdVec4i(~0), ezSimdVec4i(0));
}

static FORCE_INLINE ezSimdVec4i _mmw_cmpgt_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ezSimdVec4i::Select(a > b, ezSimdVec4i(~0), ezSimdVec4i(0));
}

static FORCE_INLINE ezSimdVec4i _mmw_min_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a.CompMin(b); }
static FORCE_INLINE ezSimdVec4i _mmw_max_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a.CompMax(b); }
static FORCE_INLINE ezSimdVec4i _mmw_abs_epi32(const ezSimdVec4i& a) { return a.Abs(); }
static FORCE_INLINE ezSimdVec4i _mmw_mullo_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a.CompMul(b); }

static FORCE_INLINE ezSimdVec4i _mmw_cvtps_epi32(const ezSimdVec4f& v)
{
  return ezSimdVec4i::Truncate(v.Round());
}

static FORCE_INLINE ezSimdVec4i _mmw_cvttps_epi32(const ezSimdVec4f& v)
{
  return ezSimdVec4i::Truncate(v);
}

static FORCE_INLINE ezSimdVec4i _mmw_blendv_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b, const ezSimdVec4i& mask)
{
  ezSimdVec4b cmp = mask < ezSimdVec4i(0);
  return ezSimdVec4i::Select(cmp, b, a);
}

static FORCE_INLINE ezSimdVec4i _mmw_subs_epu16(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return (a - b).CompMax(ezSimdVec4i(0));
}

static FORCE_INLINE int _mmw_testz_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ((a & b) == ezSimdVec4i(0)).AllSet<4>() ? 1 : 0;
}

static FORCE_INLINE ezSimdVec4i _mmw_transpose_epi8(const ezSimdVec4i& a)
{
  ezUInt8 bytes[16];
  a.Store<4>(reinterpret_cast<ezInt32*>(bytes));

  ezUInt8 result[16];
  for (int row = 0; row < 4; ++row)
    for (int col = 0; col < 4; ++col)
      result[row * 4 + col] = bytes[col * 4 + row];

  ezSimdVec4i r;
  r.Load<4>(reinterpret_cast<const ezInt32*>(result));
  return r;
}

static FORCE_INLINE ezSimdVec4i _mmw_sllv_ones(const ezSimdVec4i& ishift)
{
  ezSimdVec4i shift = ishift.CompMin(ezSimdVec4i(32));

  static const unsigned int maskLUT[33] = {
    ~0U << 0, ~0U << 1, ~0U << 2, ~0U << 3, ~0U << 4, ~0U << 5, ~0U << 6, ~0U << 7,
    ~0U << 8, ~0U << 9, ~0U << 10, ~0U << 11, ~0U << 12, ~0U << 13, ~0U << 14, ~0U << 15,
    ~0U << 16, ~0U << 17, ~0U << 18, ~0U << 19, ~0U << 20, ~0U << 21, ~0U << 22, ~0U << 23,
    ~0U << 24, ~0U << 25, ~0U << 26, ~0U << 27, ~0U << 28, ~0U << 29, ~0U << 30, ~0U << 31,
    0U};

  ezInt32 sv[4];
  shift.Store<4>(sv);

  ezInt32 result[4];
  result[0] = (ezInt32)maskLUT[sv[0]];
  result[1] = (ezInt32)maskLUT[sv[1]];
  result[2] = (ezInt32)maskLUT[sv[2]];
  result[3] = (ezInt32)maskLUT[sv[3]];

  ezSimdVec4i r;
  r.Load<4>(result);
  return r;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Y>
static FORCE_INLINE T simd_cast(Y A);

template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(float A) { return ezSimdVec4f(A); }
template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(ezSimdVec4i A) { return A.ReinterpretAsFloat(); }
template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(ezSimdVec4f A) { return A; }
template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(int A) { return ezSimdVec4i(A); }
template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(ezSimdVec4f A) { return A.ReinterpretAsInt(); }
template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(ezSimdVec4i A) { return A; }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Per-lane access
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE float* simd_f32(ezSimdVec4f& a) { return reinterpret_cast<float*>(&a.m_v); }
static FORCE_INLINE const float* simd_f32(const ezSimdVec4f& a) { return reinterpret_cast<const float*>(&a.m_v); }
static FORCE_INLINE int* simd_i32(ezSimdVec4i& a) { return reinterpret_cast<int*>(&a.m_v); }
static FORCE_INLINE const int* simd_i32(const ezSimdVec4i& a) { return reinterpret_cast<const int*>(&a.m_v); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test registration - note: group already created in SSE file
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern ezSimpleRendererTestGroup g_SimpleRendererTestGroup__MOCSIMDTest;

#define MOC_TEST_PREFIX EZ

#include "MOCSIMDTests.inl"
