// ezEngine SIMD implementation of _mmw_* macros for comparison testing.
// This sets up the ez SIMD abstraction and then includes the shared tests.

#include <RendererTest/RendererTestPCH.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

#include <Foundation/SimdMath/SimdVec4b.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/SimdMath/SimdVec4u.h>

#if defined(EZ_RENDERERTEST_MOC_SUPPORT)
#  include <RendererCore/Rasterizer/GenericMOC/GenericMOC.h>
#endif

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
  return ezSimdVec4i(ezSimdVec4u(a).SubtractSaturated16(ezSimdVec4u(b)));
}

static FORCE_INLINE int _mmw_testz_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ((a & b) == ezSimdVec4i(0)).AllSet<4>() ? 1 : 0;
}

static FORCE_INLINE ezSimdVec4i _mmw_transpose_epi8(const ezSimdVec4i& a)
{
  return a.TransposeBytes();
}

static FORCE_INLINE ezSimdVec4i _mmw_sllv_ones(const ezSimdVec4i& ishift)
{
  return ezSimdVec4i(ezSimdVec4u(0xFFFFFFFFu) << ezSimdVec4u(ishift));
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

#if EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
using MocAliasedFloat = float __attribute__((may_alias));
using MocAliasedInt = int __attribute__((may_alias));
#else
using MocAliasedFloat = float;
using MocAliasedInt = int;
#endif

static FORCE_INLINE MocAliasedFloat* simd_f32(ezSimdVec4f& a) { return reinterpret_cast<MocAliasedFloat*>(&a.m_v); }
static FORCE_INLINE const MocAliasedFloat* simd_f32(const ezSimdVec4f& a) { return reinterpret_cast<const MocAliasedFloat*>(&a.m_v); }
static FORCE_INLINE MocAliasedInt* simd_i32(ezSimdVec4i& a) { return reinterpret_cast<MocAliasedInt*>(&a.m_v); }
static FORCE_INLINE const MocAliasedInt* simd_i32(const ezSimdVec4i& a) { return reinterpret_cast<const MocAliasedInt*>(&a.m_v); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test registration - note: group already created in SSE file
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern ezSimpleRendererTestGroup g_SimpleRendererTestGroup__MOCSIMDTest;

#define MOC_TEST_PREFIX EZ

#include "MOCSIMDTests.inl"

#if defined(EZ_RENDERERTEST_MOC_SUPPORT)
EZ_CREATE_SIMPLE_RENDERER_TEST(MOCSIMDTest, GenericMOCLifetime)
{
  for (ezUInt32 i = 0; i < 3; ++i)
  {
    MaskedOcclusionCulling* pMOC = ezCreateGenericMOC();
    EZ_TEST_BOOL(pMOC != nullptr);
    if (pMOC != nullptr)
      ezDestroyGenericMOC(pMOC);
  }
}
#endif
