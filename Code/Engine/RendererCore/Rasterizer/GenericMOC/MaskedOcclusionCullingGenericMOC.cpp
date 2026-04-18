// Generic (portable) implementation of Intel's Masked Occlusion Culling.
// Translates the SSE2 implementation to use ezEngine's SIMD abstraction layer,
// enabling MOC to run on x86 (SSE2), ARM (NEON), and scalar (FPU) backends.
//
// Strategy: We redirect the raw __m128/__m128i types and _mm_* intrinsics used
// in MaskedOcclusionCullingCommon.inl to our own ez-SIMD-based inline functions.
// On x86 these names conflict with the real SSE headers, so we must prevent
// immintrin.h from being included. We achieve this by NOT including the PCH
// and providing our own definitions.

// Skip the PCH to avoid conflicts but still need core ez types.
// Foundation/Basics.h is always available and pulls in platform detection.

#include <Foundation/Basics.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4b.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/SimdMath/SimdVec4u.h>

// MaskedOcclusionCulling.h is the public API (no SSE deps)
#include <MaskedOcclusionCulling.h>

#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <new>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Compiler compatibility
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define FORCE_INLINE EZ_ALWAYS_INLINE

static FORCE_INLINE unsigned long find_clear_lsb(unsigned int* mask)
{
  unsigned long idx = 0;
  unsigned int val = *mask;
  while (idx < 32 && !(val & (1u << idx)))
    ++idx;
  *mask = val & (val - 1);
  return idx;
}

static FORCE_INLINE void* moc_aligned_alloc(size_t alignment, size_t size)
{
  return ezFoundation::GetAlignedAllocator()->Allocate(size, static_cast<ezUInt32>(alignment));
}

static FORCE_INLINE void moc_aligned_free(void* ptr)
{
  ezFoundation::GetAlignedAllocator()->Deallocate(ptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Typedefs and forward declarations
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef MaskedOcclusionCulling::pfnAlignedAlloc pfnAlignedAlloc;
typedef MaskedOcclusionCulling::pfnAlignedFree pfnAlignedFree;
typedef MaskedOcclusionCulling::VertexLayout VertexLayout;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD configuration for 4-lane generic implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANES 4
#define TILE_HEIGHT_SHIFT 2

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Types: map __mw/__mwi to ezSimd types, __m128/__m128i also to the same types
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef ezSimdVec4f __mw;
typedef ezSimdVec4i __mwi;

// The Common.inl also uses raw __m128/__m128i for 128-bit operations (frustum planes, etc.)
// In the generic copy these have been renamed to __mocm128/__mocm128i.
typedef ezSimdVec4f __mocm128;
typedef ezSimdVec4i __mocm128i;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lane index and tile layout constants
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define SIMD_LANE_IDX ezSimdVec4i(0, 1, 2, 3)

#define SIMD_SUB_TILE_COL_OFFSET ezSimdVec4i(0, SUB_TILE_WIDTH, SUB_TILE_WIDTH * 2, SUB_TILE_WIDTH * 3)
#define SIMD_SUB_TILE_ROW_OFFSET ezSimdVec4i(0, 0, 0, 0)
#define SIMD_SUB_TILE_COL_OFFSET_F ezSimdVec4f(0.0f, (float)SUB_TILE_WIDTH, (float)(SUB_TILE_WIDTH * 2), (float)(SUB_TILE_WIDTH * 3))
#define SIMD_SUB_TILE_ROW_OFFSET_F ezSimdVec4f(0.0f, 0.0f, 0.0f, 0.0f)

#define SIMD_LANE_YCOORD_I ezSimdVec4i(128, 384, 640, 896)
#define SIMD_LANE_YCOORD_F ezSimdVec4f(128.0f, 384.0f, 640.0f, 896.0f)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Float operations (_mmw_ prefix -> ezSimdVec4f)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_ps(x) ezSimdVec4f(x)
#define _mmw_setzero_ps() ezSimdVec4f::MakeZero()

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

static FORCE_INLINE ezSimdVec4f _mmw_not_ps(const ezSimdVec4f& a)
{
  return (~a.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_andnot_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  // andnot(a,b) = ~a & b
  return (~a.ReinterpretAsInt() & b.ReinterpretAsInt()).ReinterpretAsFloat();
}

static FORCE_INLINE ezSimdVec4f _mmw_neg_ps(const ezSimdVec4f& a)
{
  return -a;
}

static FORCE_INLINE ezSimdVec4f _mmw_abs_ps(const ezSimdVec4f& a)
{
  return a.Abs();
}

#define _mmw_add_ps(a, b) ((a) + (b))
#define _mmw_sub_ps(a, b) ((a) - (b))

static FORCE_INLINE ezSimdVec4f _mmw_mul_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a.CompMul(b);
}

static FORCE_INLINE ezSimdVec4f _mmw_div_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a.CompDiv(b);
}

static FORCE_INLINE ezSimdVec4f _mmw_min_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a.CompMin(b);
}

static FORCE_INLINE ezSimdVec4f _mmw_max_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a.CompMax(b);
}

// Comparisons return float masks (all-1 or all-0 bits per lane)
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

// blendv: select b where mask sign bit is set, else a
static FORCE_INLINE ezSimdVec4f _mmw_blendv_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& mask)
{
  ezSimdVec4b cmp = mask.ReinterpretAsInt() < ezSimdVec4i(0); // sign bit test
  return ezSimdVec4f::Select(cmp, b, a);
}

static FORCE_INLINE int _mmw_movemask_ps(const ezSimdVec4f& v)
{
  ezSimdVec4b negative = v.ReinterpretAsInt() < ezSimdVec4i(0);
  return (int)negative.GetMoveMask();
}

static FORCE_INLINE ezSimdVec4f _mmw_floor_ps(const ezSimdVec4f& a)
{
  return a.Floor();
}

static FORCE_INLINE ezSimdVec4f _mmw_ceil_ps(const ezSimdVec4f& a)
{
  return a.Ceil();
}

// Shuffle: _mm_shuffle_ps(a, b, imm8) - pick 2 from a, 2 from b based on immediate
static FORCE_INLINE ezSimdVec4f _mmw_shuffle_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, int imm)
{
  float va[4], vb[4], result[4];
  a.Store<4>(va);
  b.Store<4>(vb);
  result[0] = va[(imm >> 0) & 3];
  result[1] = va[(imm >> 2) & 3];
  result[2] = vb[(imm >> 4) & 3];
  result[3] = vb[(imm >> 6) & 3];
  ezSimdVec4f r;
  r.Load<4>(result);
  return r;
}

// For SSE path, insertf32x4 is identity (single 128-bit register)
#define _mmw_insertf32x4_ps(a, b, c) (b)

#define _mmw_cvtepi32_ps(v) ((v).ToFloat())

static FORCE_INLINE ezSimdVec4i _mmw_blendv_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b, const ezSimdVec4i& mask)
{
  ezSimdVec4b cmp = mask < ezSimdVec4i(0);
  return ezSimdVec4i::Select(cmp, b, a);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Integer operations (_mmw_ prefix -> ezSimdVec4i)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _mmw_set1_epi32(x) ezSimdVec4i(x)
#define _mmw_setzero_epi32() ezSimdVec4i(0)

static FORCE_INLINE ezSimdVec4i _mmw_and_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a & b; }
static FORCE_INLINE ezSimdVec4i _mmw_or_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a | b; }
static FORCE_INLINE ezSimdVec4i _mmw_xor_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a ^ b; }
static FORCE_INLINE ezSimdVec4i _mmw_not_epi32(const ezSimdVec4i& a) { return ~a; }

static FORCE_INLINE ezSimdVec4i _mmw_andnot_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ~a & b;
}

static FORCE_INLINE ezSimdVec4i _mmw_neg_epi32(const ezSimdVec4i& a) { return -a; }
static FORCE_INLINE ezSimdVec4i _mmw_add_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a + b; }
static FORCE_INLINE ezSimdVec4i _mmw_sub_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b) { return a - b; }

static FORCE_INLINE ezSimdVec4i _mmw_srai_epi32(const ezSimdVec4i& v, int imm) { return v >> (ezUInt32)imm; }
static FORCE_INLINE ezSimdVec4i _mmw_srli_epi32(const ezSimdVec4i& v, int imm)
{
  // Logical right shift: use unsigned cast
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
  // Round to nearest integer (banker's rounding) — approximate with round then truncate
  return ezSimdVec4i::Truncate(v.Round());
}

static FORCE_INLINE ezSimdVec4i _mmw_cvttps_epi32(const ezSimdVec4f& v)
{
  return ezSimdVec4i::Truncate(v);
}

// Saturating unsigned 16-bit subtract: clamp(a - b, 0, ...)
// The values are packed as 32-bit ints but guaranteed to fit in 16 bits.
// Equivalent: max(a - b, 0) per 32-bit lane
static FORCE_INLINE ezSimdVec4i _mmw_subs_epu16(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return (a - b).CompMax(ezSimdVec4i(0));
}

// testz: returns 1 if (a & b) == 0 for all bits
static FORCE_INLINE int _mmw_testz_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return ((a & b) == ezSimdVec4i(0)).AllSet<4>() ? 1 : 0;
}

// Byte transpose across the 128-bit register: transposes the 4x4 matrix of bytes.
// Input bytes:  [a0 a1 a2 a3] [b0 b1 b2 b3] [c0 c1 c2 c3] [d0 d1 d2 d3]
// Output bytes: [a0 b0 c0 d0] [a1 b1 c1 d1] [a2 b2 c2 d2] [a3 b3 c3 d3]
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

// Dynamic per-lane left shift of all-ones mask
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
// Raw __mocm128/__mocm128i intrinsic replacements (used in the modified Common.inl)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE ezSimdVec4f _moc_mm_setr_ps(float x, float y, float z, float w)
{
  return ezSimdVec4f(x, y, z, w);
}

static FORCE_INLINE ezSimdVec4i _moc_mm_setr_epi32(int x, int y, int z, int w)
{
  return ezSimdVec4i(x, y, z, w);
}

static FORCE_INLINE ezSimdVec4f _moc_mm_loadu_ps(const float* p)
{
  ezSimdVec4f v;
  v.Load<4>(p);
  return v;
}

static FORCE_INLINE ezSimdVec4i _moc_mm_setzero_si128()
{
  return ezSimdVec4i(0);
}

static FORCE_INLINE int _moc_mm_movemask_ps(const ezSimdVec4f& v)
{
  return _mmw_movemask_ps(v);
}

static FORCE_INLINE ezSimdVec4i _moc_mm_cvttps_epi32(const ezSimdVec4f& v)
{
  return _mmw_cvttps_epi32(v);
}

static FORCE_INLINE ezSimdVec4i _moc_mm_and_si128(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return a & b;
}

static FORCE_INLINE ezSimdVec4i _moc_mm_add_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return a + b;
}

static FORCE_INLINE ezSimdVec4f _moc_mm_div_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a.CompDiv(b);
}

static FORCE_INLINE ezSimdVec4f _moc_mm_sub_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return a - b;
}

static FORCE_INLINE ezSimdVec4f _moc_mm_xor_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  return _mmw_xor_ps(a, b);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// _mmx_ prefix macros (128-bit always, used for clipping/TestRect)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE ezSimdVec4f _mmx_fmadd_ps(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return ezSimdVec4f::MulAdd(a, b, c);
}

static FORCE_INLINE ezSimdVec4i _mmx_max_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return a.CompMax(b);
}

static FORCE_INLINE ezSimdVec4i _mmx_min_epi32(const ezSimdVec4i& a, const ezSimdVec4i& b)
{
  return a.CompMin(b);
}

// Dot product of all 4 components, broadcast to all lanes
static FORCE_INLINE ezSimdVec4f _mmx_dp4_ps(const ezSimdVec4f& a, const ezSimdVec4f& b)
{
  ezSimdFloat dot = a.Dot<4>(b);
  return ezSimdVec4f(dot);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SIMD casting functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename Y>
static FORCE_INLINE T simd_cast(Y A);

template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(float A)
{
  return ezSimdVec4f(A);
}

template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(ezSimdVec4i A)
{
  return A.ReinterpretAsFloat();
}

template <>
FORCE_INLINE ezSimdVec4f simd_cast<ezSimdVec4f>(ezSimdVec4f A)
{
  return A;
}

template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(int A)
{
  return ezSimdVec4i(A);
}

template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(ezSimdVec4f A)
{
  return A.ReinterpretAsInt();
}

template <>
FORCE_INLINE ezSimdVec4i simd_cast<ezSimdVec4i>(ezSimdVec4i A)
{
  return A;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Per-lane scalar access (simd_f32 / simd_i32)
// Returns pointer to the underlying data for read/write access.
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE float* simd_f32(ezSimdVec4f& a) { return reinterpret_cast<float*>(&a.m_v); }
static FORCE_INLINE const float* simd_f32(const ezSimdVec4f& a) { return reinterpret_cast<const float*>(&a.m_v); }
static FORCE_INLINE int* simd_i32(ezSimdVec4i& a) { return reinterpret_cast<int*>(&a.m_v); }
static FORCE_INLINE const int* simd_i32(const ezSimdVec4i& a) { return reinterpret_cast<const int*>(&a.m_v); }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Specialized vertex gather function for generic implementation
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static FORCE_INLINE void GatherVertices(ezSimdVec4f* vtxX, ezSimdVec4f* vtxY, ezSimdVec4f* vtxW,
  const float* inVtx, const unsigned int* inTrisPtr, int numLanes, const VertexLayout& vtxLayout)
{
  for (int lane = 0; lane < numLanes; lane++)
  {
    for (int i = 0; i < 3; i++)
    {
      const char* vPtrX = (const char*)inVtx + inTrisPtr[lane * 3 + i] * vtxLayout.mStride;
      const char* vPtrY = vPtrX + vtxLayout.mOffsetY;
      const char* vPtrW = vPtrX + vtxLayout.mOffsetW;

      simd_f32(vtxX[i])[lane] = *((const float*)vPtrX);
      simd_f32(vtxY[i])[lane] = *((const float*)vPtrY);
      simd_f32(vtxW[i])[lane] = *((const float*)vPtrW);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespace and Common.inl inclusion
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace MaskedOcclusionCullingGeneric
{
  static MaskedOcclusionCulling::Implementation gInstructionSet = MaskedOcclusionCulling::SSE2; // Report as SSE2 level

// Redirect bare __m128/__m128i types and _mm_* intrinsics to our ez-based replacements.
// Foundation headers have already been processed above, so they keep using the real SSE types.
// Only code included AFTER these defines (i.e. Common.inl) is affected.
#define __m128 __mocm128
#define __m128i __mocm128i
#define _mm_setr_ps _moc_mm_setr_ps
#define _mm_setr_epi32 _moc_mm_setr_epi32
#define _mm_loadu_ps _moc_mm_loadu_ps
#define _mm_setzero_si128 _moc_mm_setzero_si128
#define _mm_movemask_ps _moc_mm_movemask_ps
#define _mm_cvttps_epi32 _moc_mm_cvttps_epi32
#define _mm_and_si128 _moc_mm_and_si128
#define _mm_add_epi32 _moc_mm_add_epi32
#define _mm_div_ps _moc_mm_div_ps
#define _mm_sub_ps _moc_mm_sub_ps
#define _mm_xor_ps _moc_mm_xor_ps

#include <MaskedOcclusionCullingCommon.inl>

#undef __m128
#undef __m128i
#undef _mm_setr_ps
#undef _mm_setr_epi32
#undef _mm_loadu_ps
#undef _mm_setzero_si128
#undef _mm_movemask_ps
#undef _mm_cvttps_epi32
#undef _mm_and_si128
#undef _mm_add_epi32
#undef _mm_div_ps
#undef _mm_sub_ps
#undef _mm_xor_ps

  MaskedOcclusionCulling* CreateMaskedOcclusionCulling(pfnAlignedAlloc alignedAlloc, pfnAlignedFree alignedFree)
  {
    MaskedOcclusionCullingPrivate* object = (MaskedOcclusionCullingPrivate*)alignedAlloc(64, sizeof(MaskedOcclusionCullingPrivate));
    new (object) MaskedOcclusionCullingPrivate(alignedAlloc, alignedFree);
    return object;
  }
} // namespace MaskedOcclusionCullingGeneric

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public factory function
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MaskedOcclusionCulling* ezCreateGenericMOC()
{
  return MaskedOcclusionCullingGeneric::CreateMaskedOcclusionCulling(moc_aligned_alloc, moc_aligned_free);
}
