#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/Rasterizer.h>

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)

#  include <algorithm>
#  include <cassert>
#  include <cmath>

static constexpr float floatCompressionBias = 2.5237386e-29f; // 0xFFFF << 12 reinterpreted as float
static constexpr float minEdgeOffset = -0.45f;
static const float maxInvW = std::sqrt(std::numeric_limits<float>::max());

static constexpr int OFFSET_QUANTIZATION_BITS = 6;
static constexpr int OFFSET_QUANTIZATION_FACTOR = 1 << OFFSET_QUANTIZATION_BITS;

static constexpr int SLOPE_QUANTIZATION_BITS = 6;
static constexpr int SLOPE_QUANTIZATION_FACTOR = 1 << SLOPE_QUANTIZATION_BITS;

enum PrimitiveMode
{
  Culled = 0,
  Triangle0,
  Triangle1,
  ConcaveRight,
  ConcaveLeft,
  ConcaveCenter,
  Convex
};

static constexpr int modeTable[256] =
  {
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Culled,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveCenter,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveRight,
    Culled,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle1,
    Triangle1,
    Triangle1,
    Triangle1,
    Culled,
    Culled,
    Culled,
    Culled,
    Triangle1,
    Triangle1,
    Triangle1,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveRight,
    Culled,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    Culled,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Convex,
    Triangle1,
    ConcaveLeft,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    ConcaveRight,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Culled,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Culled,
    ConcaveLeft,
    Triangle1,
    ConcaveLeft,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Triangle1,
    ConcaveCenter,
    Triangle1,
    Triangle0,
    Culled,
    Triangle0,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
    Culled,
};

Rasterizer::Rasterizer(uint32_t width, uint32_t height)
  : m_width(width)
  , m_height(height)
  , m_blocksX(width / 8)
  , m_blocksY(height / 8)
{
  assert(width % 8 == 0 && height % 8 == 0);

  m_depthBuffer.resize(width * height / 8);
  m_hiZ.resize(m_blocksX * m_blocksY + 8, 0); // Add some extra padding to support out-of-bounds reads

  precomputeRasterizationTable();
}

void Rasterizer::setModelViewProjection(const float* matrix)
{
  __m128 mat0 = _mm_loadu_ps(matrix + 0);
  __m128 mat1 = _mm_loadu_ps(matrix + 4);
  __m128 mat2 = _mm_loadu_ps(matrix + 8);
  __m128 mat3 = _mm_loadu_ps(matrix + 12);

  _MM_TRANSPOSE4_PS(mat0, mat1, mat2, mat3);

  // Store rows
  _mm_storeu_ps(m_modelViewProjectionRaw + 0, mat0);
  _mm_storeu_ps(m_modelViewProjectionRaw + 4, mat1);
  _mm_storeu_ps(m_modelViewProjectionRaw + 8, mat2);
  _mm_storeu_ps(m_modelViewProjectionRaw + 12, mat3);

  // Bake viewport transform into matrix and 6shift by half a block
  mat0 = _mm_mul_ps(_mm_add_ps(mat0, mat3), _mm_set1_ps(m_width * 0.5f - 4.0f));
  mat1 = _mm_mul_ps(_mm_add_ps(mat1, mat3), _mm_set1_ps(m_height * 0.5f - 4.0f));

  // Map depth from [-1, 1] to [bias, 0]
  mat2 = _mm_mul_ps(_mm_sub_ps(mat3, mat2), _mm_set1_ps(0.5f * floatCompressionBias));

  _MM_TRANSPOSE4_PS(mat0, mat1, mat2, mat3);

  // Store prebaked cols
  _mm_storeu_ps(m_modelViewProjection + 0, mat0);
  _mm_storeu_ps(m_modelViewProjection + 4, mat1);
  _mm_storeu_ps(m_modelViewProjection + 8, mat2);
  _mm_storeu_ps(m_modelViewProjection + 12, mat3);
}

void Rasterizer::clear()
{
  // Mark blocks as cleared by setting Hi Z to 1 (one unit separated from far plane).
  // This value is extremely unlikely to occur during normal rendering, so we don't
  // need to guard against a HiZ of 1 occuring naturally. This is different from a value of 0,
  // which will occur every time a block is partially covered for the first time.
  __m128i clearValue = _mm_set1_epi16(1);
  uint32_t count = static_cast<uint32_t>(m_hiZ.size()) / 8;
  __m128i* pHiZ = reinterpret_cast<__m128i*>(m_hiZ.data());
  for (uint32_t offset = 0; offset < count; ++offset)
  {
    _mm_storeu_si128(pHiZ, clearValue);
    pHiZ++;
  }
}

bool Rasterizer::queryVisibility(__m128 boundsMin, __m128 boundsMax, bool& needsClipping)
{
  // Frustum culling is not necessary, because EZ only calls this functions for objects that are definitely inside the frustum
  //
  // Frustum cull
  __m128 extents = _mm_sub_ps(boundsMax, boundsMin);
  //__m128 center = _mm_add_ps(boundsMax, boundsMin); // Bounding box center times 2 - but since W = 2, the plane equations work out correctly
  __m128 minusZero = _mm_set1_ps(-0.0f);

  //__m128 row0 = _mm_loadu_ps(m_modelViewProjectionRaw + 0);
  //__m128 row1 = _mm_loadu_ps(m_modelViewProjectionRaw + 4);
  //__m128 row2 = _mm_loadu_ps(m_modelViewProjectionRaw + 8);
  //__m128 row3 = _mm_loadu_ps(m_modelViewProjectionRaw + 12);

  //// Compute distance from each frustum plane
  //__m128 plane0 = _mm_add_ps(row3, row0);
  //__m128 offset0 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane0, minusZero)));
  //__m128 dist0 = _mm_dp_ps(plane0, offset0, 0xff);

  //__m128 plane1 = _mm_sub_ps(row3, row0);
  //__m128 offset1 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane1, minusZero)));
  //__m128 dist1 = _mm_dp_ps(plane1, offset1, 0xff);

  //__m128 plane2 = _mm_add_ps(row3, row1);
  //__m128 offset2 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane2, minusZero)));
  //__m128 dist2 = _mm_dp_ps(plane2, offset2, 0xff);

  //__m128 plane3 = _mm_sub_ps(row3, row1);
  //__m128 offset3 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane3, minusZero)));
  //__m128 dist3 = _mm_dp_ps(plane3, offset3, 0xff);

  //__m128 plane4 = _mm_add_ps(row3, row2);
  //__m128 offset4 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane4, minusZero)));
  //__m128 dist4 = _mm_dp_ps(plane4, offset4, 0xff);

  //__m128 plane5 = _mm_sub_ps(row3, row2);
  //__m128 offset5 = _mm_add_ps(center, _mm_xor_ps(extents, _mm_and_ps(plane5, minusZero)));
  //__m128 dist5 = _mm_dp_ps(plane5, offset5, 0xff);

  //// Combine plane distance signs
  //__m128 combined = _mm_or_ps(_mm_or_ps(_mm_or_ps(dist0, dist1), _mm_or_ps(dist2, dist3)), _mm_or_ps(dist4, dist5));

  //// Can't use _mm_testz_ps or _mm_comile_ss here because the OR's above created garbage in the non-sign bits
  // if (_mm_movemask_ps(combined))
  //{
  //   return false;
  // }

  // Load prebaked projection matrix
  __m128 col0 = _mm_loadu_ps(m_modelViewProjection + 0);
  __m128 col1 = _mm_loadu_ps(m_modelViewProjection + 4);
  __m128 col2 = _mm_loadu_ps(m_modelViewProjection + 8);
  __m128 col3 = _mm_loadu_ps(m_modelViewProjection + 12);

  // Transform edges
  __m128 egde0 = _mm_mul_ps(col0, _mm_broadcastss_ps(extents));
  __m128 egde1 = _mm_mul_ps(col1, _mm_permute_ps(extents, _MM_SHUFFLE(1, 1, 1, 1)));
  __m128 egde2 = _mm_mul_ps(col2, _mm_permute_ps(extents, _MM_SHUFFLE(2, 2, 2, 2)));

  __m128 corners[8];

  // Transform first corner
  corners[0] =
    _mm_fmadd_ps(col0, _mm_broadcastss_ps(boundsMin),
      _mm_fmadd_ps(col1, _mm_permute_ps(boundsMin, _MM_SHUFFLE(1, 1, 1, 1)),
        _mm_fmadd_ps(col2, _mm_permute_ps(boundsMin, _MM_SHUFFLE(2, 2, 2, 2)),
          col3)));

  // Transform remaining corners by adding edge vectors
  corners[1] = _mm_add_ps(corners[0], egde0);
  corners[2] = _mm_add_ps(corners[0], egde1);
  corners[4] = _mm_add_ps(corners[0], egde2);

  corners[3] = _mm_add_ps(corners[1], egde1);
  corners[5] = _mm_add_ps(corners[4], egde0);
  corners[6] = _mm_add_ps(corners[2], egde2);

  corners[7] = _mm_add_ps(corners[6], egde0);

  // Transpose into SoA
  _MM_TRANSPOSE4_PS(corners[0], corners[1], corners[2], corners[3]);
  _MM_TRANSPOSE4_PS(corners[4], corners[5], corners[6], corners[7]);

  // Even if all bounding box corners have W > 0 here, we may end up with some vertices with W < 0 to due floating point differences; so test with some epsilon if any W < 0.
  __m128 maxExtent = _mm_max_ps(extents, _mm_permute_ps(extents, _MM_SHUFFLE(1, 0, 3, 2)));
  maxExtent = _mm_max_ps(maxExtent, _mm_permute_ps(maxExtent, _MM_SHUFFLE(2, 3, 0, 1)));
  __m128 nearPlaneEpsilon = _mm_mul_ps(maxExtent, _mm_set1_ps(0.001f));
  __m128 closeToNearPlane = _mm_or_ps(_mm_cmplt_ps(corners[3], nearPlaneEpsilon), _mm_cmplt_ps(corners[7], nearPlaneEpsilon));
  if (!_mm_testz_ps(closeToNearPlane, closeToNearPlane))
  {
    needsClipping = true;
    return true;
  }

  needsClipping = false;

  // Perspective division
  corners[3] = _mm_rcp_ps(corners[3]);
  corners[0] = _mm_mul_ps(corners[0], corners[3]);
  corners[1] = _mm_mul_ps(corners[1], corners[3]);
  corners[2] = _mm_mul_ps(corners[2], corners[3]);

  corners[7] = _mm_rcp_ps(corners[7]);
  corners[4] = _mm_mul_ps(corners[4], corners[7]);
  corners[5] = _mm_mul_ps(corners[5], corners[7]);
  corners[6] = _mm_mul_ps(corners[6], corners[7]);

  // Vertical mins and maxes
  __m128 minsX = _mm_min_ps(corners[0], corners[4]);
  __m128 maxsX = _mm_max_ps(corners[0], corners[4]);

  __m128 minsY = _mm_min_ps(corners[1], corners[5]);
  __m128 maxsY = _mm_max_ps(corners[1], corners[5]);


  // Horizontal reduction, step 1
  __m128 minsXY = _mm_min_ps(_mm_unpacklo_ps(minsX, minsY), _mm_unpackhi_ps(minsX, minsY));
  __m128 maxsXY = _mm_max_ps(_mm_unpacklo_ps(maxsX, maxsY), _mm_unpackhi_ps(maxsX, maxsY));

  // TODO: inflate bbox artificially to prevent incorrect occlusion due to low precision
  constexpr uint32_t inc = 2;
  minsXY = _mm_sub_ps(minsXY, _mm_setr_ps(inc, inc, inc, inc));
  maxsXY = _mm_add_ps(maxsXY, _mm_setr_ps(inc, inc, inc, inc));

  // Clamp bounds
  minsXY = _mm_max_ps(minsXY, _mm_setzero_ps());
  maxsXY = _mm_min_ps(maxsXY, _mm_setr_ps(float(m_width - 1), float(m_height - 1), float(m_width - 1), float(m_height - 1)));

  // Negate maxes so we can round in the same direction
  maxsXY = _mm_xor_ps(maxsXY, minusZero);

  // Horizontal reduction, step 2
  __m128 boundsF = _mm_min_ps(_mm_unpacklo_ps(minsXY, maxsXY), _mm_unpackhi_ps(minsXY, maxsXY));

  // Round towards -infinity and convert to int
  __m128i boundsI = _mm_cvttps_epi32(_mm_round_ps(boundsF, _MM_FROUND_TO_NEG_INF | _MM_FROUND_NO_EXC));

  // Store as scalars
  int bounds[4];
  _mm_storeu_si128(reinterpret_cast<__m128i*>(&bounds), boundsI);

  // Revert the sign change we did for the maxes
  bounds[1] = -bounds[1];
  bounds[3] = -bounds[3];

  // No intersection between quad and screen area
  if (bounds[0] >= bounds[1] || bounds[2] >= bounds[3])
  {
    return false;
  }

  uint32_t minX = bounds[0];
  uint32_t maxX = bounds[1];
  uint32_t minY = bounds[2];
  uint32_t maxY = bounds[3];

  __m128i depth = packDepthPremultiplied(corners[2], corners[6]);

  uint16_t maxZ = uint16_t(0xFFFF ^ _mm_extract_epi16(_mm_minpos_epu16(_mm_xor_si128(depth, _mm_set1_epi16(-1))), 0));

  if (!query2D(minX, maxX, minY, maxY, maxZ))
  {
    return false;
  }

  return true;
}

bool Rasterizer::query2D(uint32_t minX, uint32_t maxX, uint32_t minY, uint32_t maxY, uint32_t maxZ) const
{
  const uint16_t* pHiZBuffer = &*m_hiZ.begin();
  const __m128i* pDepthBuffer = &*m_depthBuffer.begin();

  uint32_t blockMinX = minX / 8;
  uint32_t blockMaxX = maxX / 8;

  uint32_t blockMinY = minY / 8;
  uint32_t blockMaxY = maxY / 8;

  __m128i maxZV = _mm_set1_epi16(uint16_t(maxZ));

  // Pretest against Hi-Z
  for (uint32_t blockY = blockMinY; blockY <= blockMaxY; ++blockY)
  {
    uint32_t startY = std::max<int32_t>(minY - 8 * blockY, 0);
    uint32_t endY = std::min<int32_t>(maxY - 8 * blockY, 7);

    const uint16_t* pHiZ = pHiZBuffer + (blockY * m_blocksX + blockMinX);
    const __m128i* pBlockDepth = pDepthBuffer + 8 * (blockY * m_blocksX + blockMinX) + startY;

    bool interiorLine = (startY == 0) && (endY == 7);

    for (uint32_t blockX = blockMinX; blockX <= blockMaxX; ++blockX, ++pHiZ, pBlockDepth += 8)
    {
      // Skip this block if it fully occludes the query box
      if (maxZ <= *pHiZ)
      {
        continue;
      }

      uint32_t startX = std::max<int32_t>(minX - blockX * 8, 0);

      uint32_t endX = std::min<int32_t>(maxX - blockX * 8, 7);

      bool interiorBlock = interiorLine && (startX == 0) && (endX == 7);

      // No pixels are masked, so there exists one where maxZ > pixelZ, and the query region is visible
      if (interiorBlock)
      {
        return true;
      }

      uint16_t rowSelector = (0xFFFF << 2 * startX) & (0xFFFF >> 2 * (7 - endX));

      const __m128i* pRowDepth = pBlockDepth;

      for (uint32_t y = startY; y <= endY; ++y)
      {
        __m128i rowDepth = *pRowDepth++;

        __m128i notVisible = _mm_cmpeq_epi16(_mm_min_epu16(rowDepth, maxZV), maxZV);

        uint32_t visiblePixelMask = ~_mm_movemask_epi8(notVisible);

        if ((rowSelector & visiblePixelMask) != 0)
        {
          return true;
        }
      }
    }
  }

  // Not visible
  return false;
}

void Rasterizer::readBackDepth(void* target) const
{
  const float bias = 3.9623753e+28f; // 1.0f / floatCompressionBias

  for (uint32_t blockY = 0; blockY < m_blocksY; ++blockY)
  {
    for (uint32_t blockX = 0; blockX < m_blocksX; ++blockX)
    {
      if (m_hiZ[blockY * m_blocksX + blockX] == 1)
      {
        for (uint32_t y = 0; y < 8; ++y)
        {
          uint8_t* dest = (uint8_t*)target + 4 * (8 * blockX + m_width * (8 * blockY + y));
          memset(dest, 0, 32);
        }
        continue;
      }

      const __m128i* source = &m_depthBuffer[8 * (blockY * m_blocksX + blockX)];
      for (uint32_t y = 0; y < 8; ++y)
      {
        uint8_t* dest = (uint8_t*)target + 4 * (8 * blockX + m_width * (8 * blockY + y));

        __m128i depthI = _mm_load_si128(source++);

        __m256i depthI256 = _mm256_slli_epi32(_mm256_cvtepu16_epi32(depthI), 12);
        __m256 depth = _mm256_mul_ps(_mm256_castsi256_ps(depthI256), _mm256_set1_ps(bias));

        __m256 linDepth = _mm256_div_ps(_mm256_set1_ps(2 * 0.25f), _mm256_sub_ps(_mm256_set1_ps(0.25f + 1000.0f), _mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(1.0f), depth), _mm256_set1_ps(1000.0f - 0.25f))));

        float linDepthA[16];
        _mm256_storeu_ps(linDepthA, linDepth);

        for (uint32_t x = 0; x < 8; ++x)
        {
          float l = linDepthA[x];
          uint32_t d = static_cast<uint32_t>(100 * 256 * l);
          uint8_t v0 = uint8_t(d / 100);
          uint8_t v1 = d % 256;

          dest[4 * x + 0] = v0;
          dest[4 * x + 1] = v1;
          dest[4 * x + 2] = 0;
          dest[4 * x + 3] = 255;
        }
      }
    }
  }
}

__forceinline float Rasterizer::decompressFloat(uint16_t depth)
{
  const float bias = 3.9623753e+28f; // 1.0f / floatCompressionBias

  union
  {
    uint32_t u;
    float f;
  };

  u = uint32_t(depth) << 12;
  return f * bias;
}

// turned this into a macro, because MSVC 2022 crashes in debug builds when passing these arguments into the (non-inlined (despite __forceinline)) function
#  define transpose256(A, B, C, D, out0)                      \
    {                                                         \
      __m128* out = out0;                                     \
                                                              \
      __m256 _Tmp3, _Tmp2, _Tmp1, _Tmp0;                      \
      _Tmp0 = _mm256_shuffle_ps(A, B, 0x44);                  \
      _Tmp2 = _mm256_shuffle_ps(A, B, 0xEE);                  \
      _Tmp1 = _mm256_shuffle_ps(C, D, 0x44);                  \
      _Tmp3 = _mm256_shuffle_ps(C, D, 0xEE);                  \
                                                              \
      __m256 A2 = _mm256_shuffle_ps(_Tmp0, _Tmp1, 0x88);      \
      __m256 B2 = _mm256_shuffle_ps(_Tmp0, _Tmp1, 0xDD);      \
      __m256 C2 = _mm256_shuffle_ps(_Tmp2, _Tmp3, 0x88);      \
      __m256 D2 = _mm256_shuffle_ps(_Tmp2, _Tmp3, 0xDD);      \
                                                              \
      _mm256_store_ps(reinterpret_cast<float*>(out + 0), A2); \
      _mm256_store_ps(reinterpret_cast<float*>(out + 2), B2); \
      _mm256_store_ps(reinterpret_cast<float*>(out + 4), C2); \
      _mm256_store_ps(reinterpret_cast<float*>(out + 6), D2); \
    }

// turned this into a macro, because MSVC 2022 crashes in debug builds when passing these arguments into the (non-inlined (despite __forceinline)) function
#  define transpose256i(A, B, C, D, out0)                          \
    {                                                              \
      __m128i* out = out0;                                         \
                                                                   \
      __m256i _Tmp3, _Tmp2, _Tmp1, _Tmp0;                          \
      _Tmp0 = _mm256_unpacklo_epi32(A, B);                         \
      _Tmp1 = _mm256_unpacklo_epi32(C, D);                         \
      _Tmp2 = _mm256_unpackhi_epi32(A, B);                         \
      _Tmp3 = _mm256_unpackhi_epi32(C, D);                         \
      __m256i A2 = _mm256_unpacklo_epi64(_Tmp0, _Tmp1);            \
      __m256i B2 = _mm256_unpackhi_epi64(_Tmp0, _Tmp1);            \
      __m256i C2 = _mm256_unpacklo_epi64(_Tmp2, _Tmp3);            \
      __m256i D2 = _mm256_unpackhi_epi64(_Tmp2, _Tmp3);            \
                                                                   \
      _mm256_store_si256(reinterpret_cast<__m256i*>(out + 0), A2); \
      _mm256_store_si256(reinterpret_cast<__m256i*>(out + 2), B2); \
      _mm256_store_si256(reinterpret_cast<__m256i*>(out + 4), C2); \
      _mm256_store_si256(reinterpret_cast<__m256i*>(out + 6), D2); \
    }

template <bool possiblyNearClipped>
__forceinline void Rasterizer::normalizeEdge(__m256& nx, __m256& ny, __m256 edgeFlipMask)
{
  __m256 minusZero = _mm256_set1_ps(-0.0f);
  __m256 invLen = _mm256_rcp_ps(_mm256_add_ps(_mm256_andnot_ps(minusZero, nx), _mm256_andnot_ps(minusZero, ny)));

  constexpr float maxOffset = -minEdgeOffset;
  __m256 mul = _mm256_set1_ps((OFFSET_QUANTIZATION_FACTOR - 1) / (maxOffset - minEdgeOffset));
  if (possiblyNearClipped)
  {
    mul = _mm256_xor_ps(mul, edgeFlipMask);
  }

  invLen = _mm256_mul_ps(mul, invLen);
  nx = _mm256_mul_ps(nx, invLen);
  ny = _mm256_mul_ps(ny, invLen);
}

__forceinline __m128i Rasterizer::quantizeSlopeLookup(__m128 nx, __m128 ny)
{
  __m128i yNeg = _mm_castps_si128(_mm_cmplt_ps(ny, _mm_setzero_ps()));

  // Remap [-1, 1] to [0, SLOPE_QUANTIZATION / 2]
  const float mul = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f;
  const float add = mul + 0.5f;

  __m128i quantizedSlope = _mm_cvttps_epi32(_mm_fmadd_ps(nx, _mm_set1_ps(mul), _mm_set1_ps(add)));
  return _mm_slli_epi32(_mm_sub_epi32(_mm_slli_epi32(quantizedSlope, 1), yNeg), OFFSET_QUANTIZATION_BITS);
}

__forceinline __m256i Rasterizer::quantizeSlopeLookup(__m256 nx, __m256 ny)
{
  __m256i yNeg = _mm256_castps_si256(_mm256_cmp_ps(ny, _mm256_setzero_ps(), _CMP_LE_OQ));

  // Remap [-1, 1] to [0, SLOPE_QUANTIZATION / 2]
  constexpr float maxOffset = -minEdgeOffset;
  const float mul = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f / ((OFFSET_QUANTIZATION_FACTOR - 1) / (maxOffset - minEdgeOffset));
  const float add = (SLOPE_QUANTIZATION_FACTOR / 2 - 1) * 0.5f + 0.5f;

  __m256i quantizedSlope = _mm256_cvttps_epi32(_mm256_fmadd_ps(nx, _mm256_set1_ps(mul), _mm256_set1_ps(add)));
  return _mm256_slli_epi32(_mm256_sub_epi32(_mm256_slli_epi32(quantizedSlope, 1), yNeg), OFFSET_QUANTIZATION_BITS);
}


__forceinline uint32_t Rasterizer::quantizeOffsetLookup(float offset)
{
  const float maxOffset = -minEdgeOffset;

  // Remap [minOffset, maxOffset] to [0, OFFSET_QUANTIZATION]
  const float mul = (OFFSET_QUANTIZATION_FACTOR - 1) / (maxOffset - minEdgeOffset);
  const float add = 0.5f - minEdgeOffset * mul;

  float lookup = offset * mul + add;
  return std::min(std::max(int32_t(lookup), 0), OFFSET_QUANTIZATION_FACTOR - 1);
}

__forceinline __m128i Rasterizer::packDepthPremultiplied(__m128 depthA, __m128 depthB)
{
  return _mm_packus_epi32(_mm_srai_epi32(_mm_castps_si128(depthA), 12), _mm_srai_epi32(_mm_castps_si128(depthB), 12));
}

__forceinline __m128i Rasterizer::packDepthPremultiplied(__m256 depth)
{
  __m256i x = _mm256_srai_epi32(_mm256_castps_si256(depth), 12);
  return _mm_packus_epi32(_mm256_castsi256_si128(x), _mm256_extracti128_si256(x, 1));
}

__forceinline __m256i Rasterizer::packDepthPremultiplied(__m256 depthA, __m256 depthB)
{
  __m256i x1 = _mm256_srai_epi32(_mm256_castps_si256(depthA), 12);
  __m256i x2 = _mm256_srai_epi32(_mm256_castps_si256(depthB), 12);

  return _mm256_packus_epi32(x1, x2);
}

uint64_t Rasterizer::transposeMask(uint64_t mask)
{
#  if 0
  uint64_t maskA = _pdep_u64(_pext_u64(mask, 0x5555555555555555ull), 0xF0F0F0F0F0F0F0F0ull);
  uint64_t maskB = _pdep_u64(_pext_u64(mask, 0xAAAAAAAAAAAAAAAAull), 0x0F0F0F0F0F0F0F0Full);
#  else
  uint64_t maskA = 0;
  uint64_t maskB = 0;
  for (uint32_t group = 0; group < 8; ++group)
  {
    for (uint32_t bit = 0; bit < 4; ++bit)
    {
      maskA |= ((mask >> (8 * group + 2 * bit + 0)) & 1) << (4 + group * 8 + bit);
      maskB |= ((mask >> (8 * group + 2 * bit + 1)) & 1) << (0 + group * 8 + bit);
    }
  }
#  endif
  return maskA | maskB;
}

std::vector<int64_t> Rasterizer::m_precomputedRasterTables;

void Rasterizer::precomputeRasterizationTable()
{
  // TODO: this really only needs to be done once
  if (!m_precomputedRasterTables.empty())
    return;

  const uint32_t angularResolution = 2000;
  const uint32_t offsetResolution = 2000;

  m_precomputedRasterTables.resize(OFFSET_QUANTIZATION_FACTOR * SLOPE_QUANTIZATION_FACTOR, 0);

  for (uint32_t i = 0; i < angularResolution; ++i)
  {
    float angle = -0.1f + 6.4f * float(i) / (angularResolution - 1);

    float nx = std::cos(angle);
    float ny = std::sin(angle);
    float l = 1.0f / (std::abs(nx) + std::abs(ny));

    nx *= l;
    ny *= l;

    uint32_t slopeLookup = _mm_extract_epi32(quantizeSlopeLookup(_mm_set1_ps(nx), _mm_set1_ps(ny)), 0);

    for (uint32_t j = 0; j < offsetResolution; ++j)
    {
      float offset = -0.6f + 1.2f * float(j) / (angularResolution - 1);

      uint32_t offsetLookup = quantizeOffsetLookup(offset);

      uint32_t lookup = slopeLookup | offsetLookup;

      uint64_t block = 0;

      for (auto x = 0; x < 8; ++x)
      {
        for (auto y = 0; y < 8; ++y)
        {
          float edgeDistance = offset + (x - 3.5f) / 8.0f * nx + (y - 3.5f) / 8.0f * ny;
          if (edgeDistance <= 0.0f)
          {
            uint32_t bitIndex = 8 * x + y;
            block |= uint64_t(1) << bitIndex;
          }
        }
      }

      m_precomputedRasterTables[lookup] |= transposeMask(block);
    }
    // For each slope, the first block should be all ones, the last all zeroes

    if (m_precomputedRasterTables[slopeLookup] != -1)
    {
      __debugbreak();
    }

    if (m_precomputedRasterTables[slopeLookup + OFFSET_QUANTIZATION_FACTOR - 1] != 0)
    {
      __debugbreak();
    }
  }
}

template <bool possiblyNearClipped>
void Rasterizer::rasterize(const Occluder& occluder)
{
  const __m256i* vertexData = occluder.m_vertexData;
  size_t packetCount = occluder.m_packetCount;

  __m256i maskY = _mm256_set1_epi32(2047 << 10);
  __m256i maskZ = _mm256_set1_epi32(1023);

  // Note that unaligned loads do not have a latency penalty on CPUs with SSE4 support
  __m128 mat0 = _mm_loadu_ps(m_modelViewProjection + 0);
  __m128 mat1 = _mm_loadu_ps(m_modelViewProjection + 4);
  __m128 mat2 = _mm_loadu_ps(m_modelViewProjection + 8);
  __m128 mat3 = _mm_loadu_ps(m_modelViewProjection + 12);

  __m128 boundsMin = occluder.m_refMin;
  __m128 boundsExtents = _mm_sub_ps(occluder.m_refMax, boundsMin);

  // Bake integer => bounding box transform into matrix
  mat3 =
    _mm_fmadd_ps(mat0, _mm_broadcastss_ps(boundsMin),
      _mm_fmadd_ps(mat1, _mm_permute_ps(boundsMin, _MM_SHUFFLE(1, 1, 1, 1)),
        _mm_fmadd_ps(mat2, _mm_permute_ps(boundsMin, _MM_SHUFFLE(2, 2, 2, 2)),
          mat3)));

  mat0 = _mm_mul_ps(mat0, _mm_mul_ps(_mm_broadcastss_ps(boundsExtents), _mm_set1_ps(1.0f / (2047ull << 21))));
  mat1 = _mm_mul_ps(mat1, _mm_mul_ps(_mm_permute_ps(boundsExtents, _MM_SHUFFLE(1, 1, 1, 1)), _mm_set1_ps(1.0f / (2047 << 10))));
  mat2 = _mm_mul_ps(mat2, _mm_mul_ps(_mm_permute_ps(boundsExtents, _MM_SHUFFLE(2, 2, 2, 2)), _mm_set1_ps(1.0f / 1023)));

  // Bias X coordinate back into positive range
  mat3 = _mm_fmadd_ps(mat0, _mm_set1_ps(1024ull << 21), mat3);

  // Skew projection to correct bleeding of Y and Z into X due to lack of masking
  mat1 = _mm_sub_ps(mat1, mat0);
  mat2 = _mm_sub_ps(mat2, mat0);

  _MM_TRANSPOSE4_PS(mat0, mat1, mat2, mat3);

  // Due to linear relationship between Z and W, it's cheaper to compute Z from W later in the pipeline than using the full projection matrix up front
  float c0, c1;
  {
    __m128 Za = _mm_permute_ps(mat2, _MM_SHUFFLE(3, 3, 3, 3));
    __m128 Zb = _mm_dp_ps(mat2, _mm_setr_ps(1 << 21, 1 << 10, 1, 1), 0xFF);

    __m128 Wa = _mm_permute_ps(mat3, _MM_SHUFFLE(3, 3, 3, 3));
    __m128 Wb = _mm_dp_ps(mat3, _mm_setr_ps(1 << 21, 1 << 10, 1, 1), 0xFF);

    _mm_store_ss(&c0, _mm_div_ps(_mm_sub_ps(Za, Zb), _mm_sub_ps(Wa, Wb)));
    _mm_store_ss(&c1, _mm_fnmadd_ps(_mm_div_ps(_mm_sub_ps(Za, Zb), _mm_sub_ps(Wa, Wb)), Wa, Za));
  }

  for (uint32_t packetIdx = 0; packetIdx < packetCount; packetIdx += 4)
  {
    // Load data - only needed once per frame, so use streaming load
    __m256i I0 = _mm256_stream_load_si256(vertexData + packetIdx + 0);
    __m256i I1 = _mm256_stream_load_si256(vertexData + packetIdx + 1);
    __m256i I2 = _mm256_stream_load_si256(vertexData + packetIdx + 2);
    __m256i I3 = _mm256_stream_load_si256(vertexData + packetIdx + 3);

    // Vertex transformation - first W, then X & Y after camera plane culling, then Z after backface culling
    __m256 Xf0 = _mm256_cvtepi32_ps(I0);
    __m256 Xf1 = _mm256_cvtepi32_ps(I1);
    __m256 Xf2 = _mm256_cvtepi32_ps(I2);
    __m256 Xf3 = _mm256_cvtepi32_ps(I3);

    __m256 Yf0 = _mm256_cvtepi32_ps(_mm256_and_si256(I0, maskY));
    __m256 Yf1 = _mm256_cvtepi32_ps(_mm256_and_si256(I1, maskY));
    __m256 Yf2 = _mm256_cvtepi32_ps(_mm256_and_si256(I2, maskY));
    __m256 Yf3 = _mm256_cvtepi32_ps(_mm256_and_si256(I3, maskY));

    __m256 Zf0 = _mm256_cvtepi32_ps(_mm256_and_si256(I0, maskZ));
    __m256 Zf1 = _mm256_cvtepi32_ps(_mm256_and_si256(I1, maskZ));
    __m256 Zf2 = _mm256_cvtepi32_ps(_mm256_and_si256(I2, maskZ));
    __m256 Zf3 = _mm256_cvtepi32_ps(_mm256_and_si256(I3, maskZ));

    __m256 mat00 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat0) + 0);
    __m256 mat01 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat0) + 1);
    __m256 mat02 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat0) + 2);
    __m256 mat03 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat0) + 3);

    __m256 X0 = _mm256_fmadd_ps(Xf0, mat00, _mm256_fmadd_ps(Yf0, mat01, _mm256_fmadd_ps(Zf0, mat02, mat03)));
    __m256 X1 = _mm256_fmadd_ps(Xf1, mat00, _mm256_fmadd_ps(Yf1, mat01, _mm256_fmadd_ps(Zf1, mat02, mat03)));
    __m256 X2 = _mm256_fmadd_ps(Xf2, mat00, _mm256_fmadd_ps(Yf2, mat01, _mm256_fmadd_ps(Zf2, mat02, mat03)));
    __m256 X3 = _mm256_fmadd_ps(Xf3, mat00, _mm256_fmadd_ps(Yf3, mat01, _mm256_fmadd_ps(Zf3, mat02, mat03)));

    __m256 mat10 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat1) + 0);
    __m256 mat11 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat1) + 1);
    __m256 mat12 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat1) + 2);
    __m256 mat13 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat1) + 3);

    __m256 Y0 = _mm256_fmadd_ps(Xf0, mat10, _mm256_fmadd_ps(Yf0, mat11, _mm256_fmadd_ps(Zf0, mat12, mat13)));
    __m256 Y1 = _mm256_fmadd_ps(Xf1, mat10, _mm256_fmadd_ps(Yf1, mat11, _mm256_fmadd_ps(Zf1, mat12, mat13)));
    __m256 Y2 = _mm256_fmadd_ps(Xf2, mat10, _mm256_fmadd_ps(Yf2, mat11, _mm256_fmadd_ps(Zf2, mat12, mat13)));
    __m256 Y3 = _mm256_fmadd_ps(Xf3, mat10, _mm256_fmadd_ps(Yf3, mat11, _mm256_fmadd_ps(Zf3, mat12, mat13)));

    __m256 mat30 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat3) + 0);
    __m256 mat31 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat3) + 1);
    __m256 mat32 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat3) + 2);
    __m256 mat33 = _mm256_broadcast_ss(reinterpret_cast<const float*>(&mat3) + 3);

    __m256 W0 = _mm256_fmadd_ps(Xf0, mat30, _mm256_fmadd_ps(Yf0, mat31, _mm256_fmadd_ps(Zf0, mat32, mat33)));
    __m256 W1 = _mm256_fmadd_ps(Xf1, mat30, _mm256_fmadd_ps(Yf1, mat31, _mm256_fmadd_ps(Zf1, mat32, mat33)));
    __m256 W2 = _mm256_fmadd_ps(Xf2, mat30, _mm256_fmadd_ps(Yf2, mat31, _mm256_fmadd_ps(Zf2, mat32, mat33)));
    __m256 W3 = _mm256_fmadd_ps(Xf3, mat30, _mm256_fmadd_ps(Yf3, mat31, _mm256_fmadd_ps(Zf3, mat32, mat33)));

    __m256 invW0, invW1, invW2, invW3;
    // Clamp W and invert
    if (possiblyNearClipped)
    {
      __m256 lowerBound = _mm256_set1_ps(-maxInvW);
      __m256 upperBound = _mm256_set1_ps(+maxInvW);
      invW0 = _mm256_min_ps(upperBound, _mm256_max_ps(lowerBound, _mm256_rcp_ps(W0)));
      invW1 = _mm256_min_ps(upperBound, _mm256_max_ps(lowerBound, _mm256_rcp_ps(W1)));
      invW2 = _mm256_min_ps(upperBound, _mm256_max_ps(lowerBound, _mm256_rcp_ps(W2)));
      invW3 = _mm256_min_ps(upperBound, _mm256_max_ps(lowerBound, _mm256_rcp_ps(W3)));
    }
    else
    {
      invW0 = _mm256_rcp_ps(W0);
      invW1 = _mm256_rcp_ps(W1);
      invW2 = _mm256_rcp_ps(W2);
      invW3 = _mm256_rcp_ps(W3);
    }

    // Round to integer coordinates to improve culling of zero-area triangles
    __m256 x0 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(X0, invW0), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 x1 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(X1, invW1), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 x2 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(X2, invW2), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 x3 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(X3, invW3), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));

    __m256 y0 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(Y0, invW0), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 y1 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(Y1, invW1), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 y2 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(Y2, invW2), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));
    __m256 y3 = _mm256_mul_ps(_mm256_round_ps(_mm256_mul_ps(Y3, invW3), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC), _mm256_set1_ps(0.125f));

    // Compute unnormalized edge directions
    __m256 edgeNormalsX0 = _mm256_sub_ps(y1, y0);
    __m256 edgeNormalsX1 = _mm256_sub_ps(y2, y1);
    __m256 edgeNormalsX2 = _mm256_sub_ps(y3, y2);
    __m256 edgeNormalsX3 = _mm256_sub_ps(y0, y3);

    __m256 edgeNormalsY0 = _mm256_sub_ps(x0, x1);
    __m256 edgeNormalsY1 = _mm256_sub_ps(x1, x2);
    __m256 edgeNormalsY2 = _mm256_sub_ps(x2, x3);
    __m256 edgeNormalsY3 = _mm256_sub_ps(x3, x0);

    __m256 area0 = _mm256_fmsub_ps(edgeNormalsX0, edgeNormalsY1, _mm256_mul_ps(edgeNormalsX1, edgeNormalsY0));
    __m256 area1 = _mm256_fmsub_ps(edgeNormalsX1, edgeNormalsY2, _mm256_mul_ps(edgeNormalsX2, edgeNormalsY1));
    __m256 area2 = _mm256_fmsub_ps(edgeNormalsX2, edgeNormalsY3, _mm256_mul_ps(edgeNormalsX3, edgeNormalsY2));
    __m256 area3 = _mm256_sub_ps(_mm256_add_ps(area0, area2), area1);

    __m256 minusZero256 = _mm256_set1_ps(-0.0f);

    __m256 wSign0, wSign1, wSign2, wSign3;
    if (possiblyNearClipped)
    {
      wSign0 = _mm256_and_ps(invW0, minusZero256);
      wSign1 = _mm256_and_ps(invW1, minusZero256);
      wSign2 = _mm256_and_ps(invW2, minusZero256);
      wSign3 = _mm256_and_ps(invW3, minusZero256);
    }
    else
    {
      wSign0 = _mm256_setzero_ps();
      wSign1 = _mm256_setzero_ps();
      wSign2 = _mm256_setzero_ps();
      wSign3 = _mm256_setzero_ps();
    }

    // Compute signs of areas. We treat 0 as negative as this allows treating primitives with zero area as backfacing.
    __m256 areaSign0, areaSign1, areaSign2, areaSign3;
    if (possiblyNearClipped)
    {
      // Flip areas for each vertex with W < 0. This needs to be done before comparison against 0 rather than afterwards to make sure zero-are triangles are handled correctly.
      areaSign0 = _mm256_cmp_ps(_mm256_xor_ps(_mm256_xor_ps(area0, wSign0), _mm256_xor_ps(wSign1, wSign2)), _mm256_setzero_ps(), _CMP_LE_OQ);
      areaSign1 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(_mm256_xor_ps(_mm256_xor_ps(area1, wSign1), _mm256_xor_ps(wSign2, wSign3)), _mm256_setzero_ps(), _CMP_LE_OQ));
      areaSign2 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(_mm256_xor_ps(_mm256_xor_ps(area2, wSign0), _mm256_xor_ps(wSign2, wSign3)), _mm256_setzero_ps(), _CMP_LE_OQ));
      areaSign3 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(_mm256_xor_ps(_mm256_xor_ps(area3, wSign1), _mm256_xor_ps(wSign0, wSign3)), _mm256_setzero_ps(), _CMP_LE_OQ));
    }
    else
    {
      areaSign0 = _mm256_cmp_ps(area0, _mm256_setzero_ps(), _CMP_LE_OQ);
      areaSign1 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(area1, _mm256_setzero_ps(), _CMP_LE_OQ));
      areaSign2 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(area2, _mm256_setzero_ps(), _CMP_LE_OQ));
      areaSign3 = _mm256_and_ps(minusZero256, _mm256_cmp_ps(area3, _mm256_setzero_ps(), _CMP_LE_OQ));
    }

    __m256i config = _mm256_or_si256(
      _mm256_or_si256(_mm256_srli_epi32(_mm256_castps_si256(areaSign3), 28), _mm256_srli_epi32(_mm256_castps_si256(areaSign2), 29)),
      _mm256_or_si256(_mm256_srli_epi32(_mm256_castps_si256(areaSign1), 30), _mm256_srli_epi32(_mm256_castps_si256(areaSign0), 31)));

    if (possiblyNearClipped)
    {
      config = _mm256_or_si256(config,
        _mm256_or_si256(
          _mm256_or_si256(_mm256_srli_epi32(_mm256_castps_si256(wSign3), 24), _mm256_srli_epi32(_mm256_castps_si256(wSign2), 25)),
          _mm256_or_si256(_mm256_srli_epi32(_mm256_castps_si256(wSign1), 26), _mm256_srli_epi32(_mm256_castps_si256(wSign0), 27))));
    }

    __m256i modes = _mm256_i32gather_epi32(modeTable, config, 4);
    if (_mm256_testz_si256(modes, modes))
    {
      continue;
    }

    __m256i primitiveValid = _mm256_cmpgt_epi32(modes, _mm256_setzero_si256());

    uint32_t primModes[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(primModes), modes);

    __m256 minFx, minFy, maxFx, maxFy;

    if (possiblyNearClipped)
    {
      // Clipless bounding box computation
      __m256 infP = _mm256_set1_ps(+10000.0f);
      __m256 infN = _mm256_set1_ps(-10000.0f);

      // Find interval of points with W > 0
      __m256 minPx0 = _mm256_blendv_ps(x0, infP, wSign0);
      __m256 minPx1 = _mm256_blendv_ps(x1, infP, wSign1);
      __m256 minPx2 = _mm256_blendv_ps(x2, infP, wSign2);
      __m256 minPx3 = _mm256_blendv_ps(x3, infP, wSign3);

      __m256 minPx = _mm256_min_ps(
        _mm256_min_ps(minPx0, minPx1),
        _mm256_min_ps(minPx2, minPx3));

      __m256 minPy0 = _mm256_blendv_ps(y0, infP, wSign0);
      __m256 minPy1 = _mm256_blendv_ps(y1, infP, wSign1);
      __m256 minPy2 = _mm256_blendv_ps(y2, infP, wSign2);
      __m256 minPy3 = _mm256_blendv_ps(y3, infP, wSign3);

      __m256 minPy = _mm256_min_ps(
        _mm256_min_ps(minPy0, minPy1),
        _mm256_min_ps(minPy2, minPy3));

      __m256 maxPx0 = _mm256_xor_ps(minPx0, wSign0);
      __m256 maxPx1 = _mm256_xor_ps(minPx1, wSign1);
      __m256 maxPx2 = _mm256_xor_ps(minPx2, wSign2);
      __m256 maxPx3 = _mm256_xor_ps(minPx3, wSign3);

      __m256 maxPx = _mm256_max_ps(
        _mm256_max_ps(maxPx0, maxPx1),
        _mm256_max_ps(maxPx2, maxPx3));

      __m256 maxPy0 = _mm256_xor_ps(minPy0, wSign0);
      __m256 maxPy1 = _mm256_xor_ps(minPy1, wSign1);
      __m256 maxPy2 = _mm256_xor_ps(minPy2, wSign2);
      __m256 maxPy3 = _mm256_xor_ps(minPy3, wSign3);

      __m256 maxPy = _mm256_max_ps(
        _mm256_max_ps(maxPy0, maxPy1),
        _mm256_max_ps(maxPy2, maxPy3));

      // Find interval of points with W < 0
      __m256 minNx0 = _mm256_blendv_ps(infP, x0, wSign0);
      __m256 minNx1 = _mm256_blendv_ps(infP, x1, wSign1);
      __m256 minNx2 = _mm256_blendv_ps(infP, x2, wSign2);
      __m256 minNx3 = _mm256_blendv_ps(infP, x3, wSign3);

      __m256 minNx = _mm256_min_ps(
        _mm256_min_ps(minNx0, minNx1),
        _mm256_min_ps(minNx2, minNx3));

      __m256 minNy0 = _mm256_blendv_ps(infP, y0, wSign0);
      __m256 minNy1 = _mm256_blendv_ps(infP, y1, wSign1);
      __m256 minNy2 = _mm256_blendv_ps(infP, y2, wSign2);
      __m256 minNy3 = _mm256_blendv_ps(infP, y3, wSign3);

      __m256 minNy = _mm256_min_ps(
        _mm256_min_ps(minNy0, minNy1),
        _mm256_min_ps(minNy2, minNy3));

      __m256 maxNx0 = _mm256_blendv_ps(infN, x0, wSign0);
      __m256 maxNx1 = _mm256_blendv_ps(infN, x1, wSign1);
      __m256 maxNx2 = _mm256_blendv_ps(infN, x2, wSign2);
      __m256 maxNx3 = _mm256_blendv_ps(infN, x3, wSign3);

      __m256 maxNx = _mm256_max_ps(
        _mm256_max_ps(maxNx0, maxNx1),
        _mm256_max_ps(maxNx2, maxNx3));

      __m256 maxNy0 = _mm256_blendv_ps(infN, y0, wSign0);
      __m256 maxNy1 = _mm256_blendv_ps(infN, y1, wSign1);
      __m256 maxNy2 = _mm256_blendv_ps(infN, y2, wSign2);
      __m256 maxNy3 = _mm256_blendv_ps(infN, y3, wSign3);

      __m256 maxNy = _mm256_max_ps(
        _mm256_max_ps(maxNy0, maxNy1),
        _mm256_max_ps(maxNy2, maxNy3));

      // Include interval bounds resp. infinity depending on ordering of intervals
      __m256 incAx = _mm256_blendv_ps(minPx, infN, _mm256_cmp_ps(maxNx, minPx, _CMP_GT_OQ));
      __m256 incAy = _mm256_blendv_ps(minPy, infN, _mm256_cmp_ps(maxNy, minPy, _CMP_GT_OQ));

      __m256 incBx = _mm256_blendv_ps(maxPx, infP, _mm256_cmp_ps(maxPx, minNx, _CMP_GT_OQ));
      __m256 incBy = _mm256_blendv_ps(maxPy, infP, _mm256_cmp_ps(maxPy, minNy, _CMP_GT_OQ));

      minFx = _mm256_min_ps(incAx, incBx);
      minFy = _mm256_min_ps(incAy, incBy);

      maxFx = _mm256_max_ps(incAx, incBx);
      maxFy = _mm256_max_ps(incAy, incBy);
    }
    else
    {
      // Standard bounding box inclusion
      minFx = _mm256_min_ps(_mm256_min_ps(x0, x1), _mm256_min_ps(x2, x3));
      minFy = _mm256_min_ps(_mm256_min_ps(y0, y1), _mm256_min_ps(y2, y3));

      maxFx = _mm256_max_ps(_mm256_max_ps(x0, x1), _mm256_max_ps(x2, x3));
      maxFy = _mm256_max_ps(_mm256_max_ps(y0, y1), _mm256_max_ps(y2, y3));
    }

    // Clamp and round
    __m256i minX, minY, maxX, maxY;
    minX = _mm256_max_epi32(_mm256_cvttps_epi32(_mm256_add_ps(minFx, _mm256_set1_ps(4.9999f / 8.0f))), _mm256_setzero_si256());
    minY = _mm256_max_epi32(_mm256_cvttps_epi32(_mm256_add_ps(minFy, _mm256_set1_ps(4.9999f / 8.0f))), _mm256_setzero_si256());
    maxX = _mm256_min_epi32(_mm256_cvttps_epi32(_mm256_add_ps(maxFx, _mm256_set1_ps(11.0f / 8.0f))), _mm256_set1_epi32(m_blocksX));
    maxY = _mm256_min_epi32(_mm256_cvttps_epi32(_mm256_add_ps(maxFy, _mm256_set1_ps(11.0f / 8.0f))), _mm256_set1_epi32(m_blocksY));

    // Check overlap between bounding box and frustum
    __m256i inFrustum = _mm256_and_si256(_mm256_cmpgt_epi32(maxX, minX), _mm256_cmpgt_epi32(maxY, minY));
    primitiveValid = _mm256_and_si256(inFrustum, primitiveValid);

    if (_mm256_testz_si256(primitiveValid, primitiveValid))
    {
      continue;
    }

    // Convert bounds from [min, max] to [min, range]
    __m256i rangeX = _mm256_sub_epi32(maxX, minX);
    __m256i rangeY = _mm256_sub_epi32(maxY, minY);

    // Compute Z from linear relation with 1/W
    __m256 z0, z1, z2, z3;
    __m256 C0 = _mm256_broadcast_ss(&c0);
    __m256 C1 = _mm256_broadcast_ss(&c1);
    z0 = _mm256_fmadd_ps(invW0, C1, C0);
    z1 = _mm256_fmadd_ps(invW1, C1, C0);
    z2 = _mm256_fmadd_ps(invW2, C1, C0);
    z3 = _mm256_fmadd_ps(invW3, C1, C0);

    __m256 maxZ = _mm256_max_ps(_mm256_max_ps(z0, z1), _mm256_max_ps(z2, z3));

    // If any W < 0, assume maxZ = 1 (effectively disabling Hi-Z)
    if (possiblyNearClipped)
    {
      maxZ = _mm256_blendv_ps(maxZ, _mm256_set1_ps(1.0f), _mm256_or_ps(_mm256_or_ps(wSign0, wSign1), _mm256_or_ps(wSign2, wSign3)));
    }

    __m128i packedDepthBounds = packDepthPremultiplied(maxZ);

    uint16_t depthBounds[8];
    _mm_storeu_si128(reinterpret_cast<__m128i*>(depthBounds), packedDepthBounds);

    // Compute screen space depth plane
    __m256 greaterArea = _mm256_cmp_ps(_mm256_andnot_ps(minusZero256, area0), _mm256_andnot_ps(minusZero256, area2), _CMP_LT_OQ);

    // Force triangle area to be picked in the relevant mode.
    __m256 modeTriangle0 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(modes, _mm256_set1_epi32(Triangle0)));
    __m256 modeTriangle1 = _mm256_castsi256_ps(_mm256_cmpeq_epi32(modes, _mm256_set1_epi32(Triangle1)));
    greaterArea = _mm256_andnot_ps(modeTriangle0, _mm256_or_ps(modeTriangle1, greaterArea));


    __m256 invArea;
    if (possiblyNearClipped)
    {
      // Do a precise divison to reduce error in depth plane. Note that the area computed here
      // differs from the rasterized region if W < 0, so it can be very small for large covered screen regions.
      invArea = _mm256_div_ps(_mm256_set1_ps(1.0f), _mm256_blendv_ps(area0, area2, greaterArea));
    }
    else
    {
      invArea = _mm256_rcp_ps(_mm256_blendv_ps(area0, area2, greaterArea));
    }

    __m256 z12 = _mm256_sub_ps(z1, z2);
    __m256 z20 = _mm256_sub_ps(z2, z0);
    __m256 z30 = _mm256_sub_ps(z3, z0);


    __m256 edgeNormalsX4 = _mm256_sub_ps(y0, y2);
    __m256 edgeNormalsY4 = _mm256_sub_ps(x2, x0);

    __m256 depthPlane0, depthPlane1, depthPlane2;
    depthPlane1 = _mm256_mul_ps(invArea, _mm256_blendv_ps(_mm256_fmsub_ps(z20, edgeNormalsX1, _mm256_mul_ps(z12, edgeNormalsX4)), _mm256_fnmadd_ps(z20, edgeNormalsX3, _mm256_mul_ps(z30, edgeNormalsX4)), greaterArea));
    depthPlane2 = _mm256_mul_ps(invArea, _mm256_blendv_ps(_mm256_fmsub_ps(z20, edgeNormalsY1, _mm256_mul_ps(z12, edgeNormalsY4)), _mm256_fnmadd_ps(z20, edgeNormalsY3, _mm256_mul_ps(z30, edgeNormalsY4)), greaterArea));

    x0 = _mm256_sub_ps(x0, _mm256_cvtepi32_ps(minX));
    y0 = _mm256_sub_ps(y0, _mm256_cvtepi32_ps(minY));

    depthPlane0 = _mm256_fnmadd_ps(x0, depthPlane1, _mm256_fnmadd_ps(y0, depthPlane2, z0));

    // If mode == Triangle0, replace edge 2 with edge 4; if mode == Triangle1, replace edge 0 with edge 4
    edgeNormalsX2 = _mm256_blendv_ps(edgeNormalsX2, edgeNormalsX4, modeTriangle0);
    edgeNormalsY2 = _mm256_blendv_ps(edgeNormalsY2, edgeNormalsY4, modeTriangle0);
    edgeNormalsX0 = _mm256_blendv_ps(edgeNormalsX0, _mm256_xor_ps(minusZero256, edgeNormalsX4), modeTriangle1);
    edgeNormalsY0 = _mm256_blendv_ps(edgeNormalsY0, _mm256_xor_ps(minusZero256, edgeNormalsY4), modeTriangle1);

    // Flip edges if W < 0
    __m256 edgeFlipMask0, edgeFlipMask1, edgeFlipMask2, edgeFlipMask3;
    if (possiblyNearClipped)
    {
      edgeFlipMask0 = _mm256_xor_ps(wSign0, _mm256_blendv_ps(wSign1, wSign2, modeTriangle1));
      edgeFlipMask1 = _mm256_xor_ps(wSign1, wSign2);
      edgeFlipMask2 = _mm256_xor_ps(wSign2, _mm256_blendv_ps(wSign3, wSign0, modeTriangle0));
      edgeFlipMask3 = _mm256_xor_ps(wSign0, wSign3);
    }
    else
    {
      edgeFlipMask0 = _mm256_setzero_ps();
      edgeFlipMask1 = _mm256_setzero_ps();
      edgeFlipMask2 = _mm256_setzero_ps();
      edgeFlipMask3 = _mm256_setzero_ps();
    }

    // Normalize edge equations for lookup
    normalizeEdge<possiblyNearClipped>(edgeNormalsX0, edgeNormalsY0, edgeFlipMask0);
    normalizeEdge<possiblyNearClipped>(edgeNormalsX1, edgeNormalsY1, edgeFlipMask1);
    normalizeEdge<possiblyNearClipped>(edgeNormalsX2, edgeNormalsY2, edgeFlipMask2);
    normalizeEdge<possiblyNearClipped>(edgeNormalsX3, edgeNormalsY3, edgeFlipMask3);

    const float maxOffset = -minEdgeOffset;
    __m256 add256 = _mm256_set1_ps(0.5f - minEdgeOffset * (OFFSET_QUANTIZATION_FACTOR - 1) / (maxOffset - minEdgeOffset));
    __m256 edgeOffsets0, edgeOffsets1, edgeOffsets2, edgeOffsets3;

    edgeOffsets0 = _mm256_fnmadd_ps(x0, edgeNormalsX0, _mm256_fnmadd_ps(y0, edgeNormalsY0, add256));
    edgeOffsets1 = _mm256_fnmadd_ps(x1, edgeNormalsX1, _mm256_fnmadd_ps(y1, edgeNormalsY1, add256));
    edgeOffsets2 = _mm256_fnmadd_ps(x2, edgeNormalsX2, _mm256_fnmadd_ps(y2, edgeNormalsY2, add256));
    edgeOffsets3 = _mm256_fnmadd_ps(x3, edgeNormalsX3, _mm256_fnmadd_ps(y3, edgeNormalsY3, add256));

    edgeOffsets1 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minX), edgeNormalsX1, edgeOffsets1);
    edgeOffsets2 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minX), edgeNormalsX2, edgeOffsets2);
    edgeOffsets3 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minX), edgeNormalsX3, edgeOffsets3);

    edgeOffsets1 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minY), edgeNormalsY1, edgeOffsets1);
    edgeOffsets2 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minY), edgeNormalsY2, edgeOffsets2);
    edgeOffsets3 = _mm256_fmadd_ps(_mm256_cvtepi32_ps(minY), edgeNormalsY3, edgeOffsets3);

    // Quantize slopes
    __m256i slopeLookups0, slopeLookups1, slopeLookups2, slopeLookups3;
    slopeLookups0 = quantizeSlopeLookup(edgeNormalsX0, edgeNormalsY0);
    slopeLookups1 = quantizeSlopeLookup(edgeNormalsX1, edgeNormalsY1);
    slopeLookups2 = quantizeSlopeLookup(edgeNormalsX2, edgeNormalsY2);
    slopeLookups3 = quantizeSlopeLookup(edgeNormalsX3, edgeNormalsY3);

    __m256i firstBlockIdx = _mm256_add_epi32(_mm256_mullo_epi16(minY, _mm256_set1_epi32(m_blocksX)), minX);

    uint32_t firstBlocks[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(firstBlocks), firstBlockIdx);

    uint32_t rangesX[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(rangesX), rangeX);

    uint32_t rangesY[8];
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(rangesY), rangeY);

    // Transpose into AoS
    __m128 depthPlane[8];
    transpose256(depthPlane0, depthPlane1, depthPlane2, _mm256_setzero_ps(), depthPlane);

    __m128 edgeNormalsX[8];
    transpose256(edgeNormalsX0, edgeNormalsX1, edgeNormalsX2, edgeNormalsX3, edgeNormalsX);

    __m128 edgeNormalsY[8];
    transpose256(edgeNormalsY0, edgeNormalsY1, edgeNormalsY2, edgeNormalsY3, edgeNormalsY);

    __m128 edgeOffsets[8];
    transpose256(edgeOffsets0, edgeOffsets1, edgeOffsets2, edgeOffsets3, edgeOffsets);

    __m128i slopeLookups[8];
    transpose256i(slopeLookups0, slopeLookups1, slopeLookups2, slopeLookups3, slopeLookups);

    uint32_t validMask = _mm256_movemask_ps(_mm256_castsi256_ps(primitiveValid));

    // Fetch data pointers since we'll manually strength-reduce memory arithmetic
    const int64_t* pTable = &*m_precomputedRasterTables.begin();
    uint16_t* pHiZBuffer = &*m_hiZ.begin();
    __m128i* pDepthBuffer = &*m_depthBuffer.begin();

    // Loop over set bits
    unsigned long primitiveIdx;
    while (_BitScanForward(&primitiveIdx, validMask))
    {
      // Clear lowest set bit in mask
      validMask &= validMask - 1;

      uint32_t primitiveIdxTransposed = ((primitiveIdx << 1) & 7) | (primitiveIdx >> 2);

      // Extract and prepare per-primitive data
      uint16_t primitiveMaxZ = depthBounds[primitiveIdx];

      __m256 depthDx = _mm256_broadcastss_ps(_mm_permute_ps(depthPlane[primitiveIdxTransposed], _MM_SHUFFLE(1, 1, 1, 1)));
      __m256 depthDy = _mm256_broadcastss_ps(_mm_permute_ps(depthPlane[primitiveIdxTransposed], _MM_SHUFFLE(2, 2, 2, 2)));

      const float depthSamplePos = -0.5f + 1.0f / 16.0f;
      __m256 lineDepth =
        _mm256_fmadd_ps(depthDx, _mm256_setr_ps(depthSamplePos + 0.0f, depthSamplePos + 0.125f, depthSamplePos + 0.25f, depthSamplePos + 0.375f, depthSamplePos + 0.0f, depthSamplePos + 0.125f, depthSamplePos + 0.25f, depthSamplePos + 0.375f),
          _mm256_fmadd_ps(depthDy, _mm256_setr_ps(depthSamplePos + 0.0f, depthSamplePos + 0.0f, depthSamplePos + 0.0f, depthSamplePos + 0.0f, depthSamplePos + 0.125f, depthSamplePos + 0.125f, depthSamplePos + 0.125f, depthSamplePos + 0.125f),
            _mm256_broadcastss_ps(depthPlane[primitiveIdxTransposed])));

      __m128i slopeLookup = slopeLookups[primitiveIdxTransposed];
      __m128 edgeNormalX = edgeNormalsX[primitiveIdxTransposed];
      __m128 edgeNormalY = edgeNormalsY[primitiveIdxTransposed];
      __m128 lineOffset = edgeOffsets[primitiveIdxTransposed];

      const uint32_t blocksX = m_blocksX;

      const uint32_t firstBlock = firstBlocks[primitiveIdx];
      const uint32_t blockRangeX = rangesX[primitiveIdx];
      const uint32_t blockRangeY = rangesY[primitiveIdx];

      uint16_t* pPrimitiveHiZ = pHiZBuffer + firstBlock;
      __m256i* pPrimitiveOut = reinterpret_cast<__m256i*>(pDepthBuffer) + 4 * firstBlock;

      uint32_t primitiveMode = primModes[primitiveIdx];

      for (uint32_t blockY = 0;
           blockY < blockRangeY;
           ++blockY,
                    pPrimitiveHiZ += blocksX,
                    pPrimitiveOut += 4 * blocksX,
                    lineDepth = _mm256_add_ps(lineDepth, depthDy),
                    lineOffset = _mm_add_ps(lineOffset, edgeNormalY))
      {
        uint16_t* pBlockRowHiZ = pPrimitiveHiZ;
        __m256i* out = pPrimitiveOut;

        __m128 offset = lineOffset;
        __m256 depth = lineDepth;

        bool anyBlockHit = false;
        for (uint32_t blockX = 0;
             blockX < blockRangeX;
             ++blockX,
                      pBlockRowHiZ += 1,
                      out += 4,
                      depth = _mm256_add_ps(depthDx, depth),
                      offset = _mm_add_ps(edgeNormalX, offset))
        {
          uint16_t hiZ = *pBlockRowHiZ;
          if (hiZ >= primitiveMaxZ)
          {
            continue;
          }

          uint64_t blockMask;
          if (primitiveMode == Convex) // 83-97%
          {
            // Simplified conservative test: combined block mask will be zero if any offset is outside of range
            __m128 anyOffsetOutsideMask = _mm_cmpge_ps(offset, _mm_set1_ps(OFFSET_QUANTIZATION_FACTOR - 1));
            if (!_mm_testz_ps(anyOffsetOutsideMask, anyOffsetOutsideMask))
            {
              if (anyBlockHit)
              {
                // Convexity implies we won't hit another block in this row and can skip to the next line.
                break;
              }
              continue;
            }

            anyBlockHit = true;

            __m128i offsetClamped = _mm_max_epi32(_mm_cvttps_epi32(offset), _mm_setzero_si128());

            static uint64_t totalBlocks = 0;
            static uint64_t containedBlocks = 0;

            __m128i lookup = _mm_or_si128(slopeLookup, offsetClamped);

            // Generate block mask
            uint64_t A = pTable[uint32_t(_mm_cvtsi128_si32(lookup))];
            uint64_t B = pTable[uint32_t(_mm_extract_epi32(lookup, 1))];
            uint64_t C = pTable[uint32_t(_mm_extract_epi32(lookup, 2))];
            uint64_t D = pTable[uint32_t(_mm_extract_epi32(lookup, 3))];

            blockMask = (A & B) & (C & D);

            // It is possible but very unlikely that blockMask == 0 if all A,B,C,D != 0 according to the conservative test above, so we skip the additional branch here.
          }
          else
          {
            __m128i offsetClamped = _mm_min_epi32(_mm_max_epi32(_mm_cvttps_epi32(offset), _mm_setzero_si128()), _mm_set1_epi32(OFFSET_QUANTIZATION_FACTOR - 1));
            __m128i lookup = _mm_or_si128(slopeLookup, offsetClamped);

            // Generate block mask
            uint64_t A = pTable[uint32_t(_mm_cvtsi128_si32(lookup))];
            uint64_t B = pTable[uint32_t(_mm_extract_epi32(lookup, 1))];
            uint64_t C = pTable[uint32_t(_mm_extract_epi32(lookup, 2))];
            uint64_t D = pTable[uint32_t(_mm_extract_epi32(lookup, 3))];

            // Switch over primitive mode. MSVC compiles this as a "sub eax, 1; jz label;" ladder, so the mode enum is ordered by descending frequency of occurence
            // to optimize branch efficiency. By ensuring we have a default case that falls through to the last possible value (ConcaveLeft if not near clipped,
            // ConcaveCenter otherwise) we avoid the last branch in the ladder.
            switch (primitiveMode)
            {
              case Triangle0: // 2.3-11%
                blockMask = A & B & C;
                break;

              case Triangle1: // 0.1-4%
                blockMask = A & C & D;
                break;

              case ConcaveRight: // 0.01-0.9%
                blockMask = (A | D) & (B & C);
                break;

              default:
                // Case ConcaveCenter can only occur if any W < 0
                if (possiblyNearClipped)
                {
                  // case ConcaveCenter:			// < 1e-6%
                  blockMask = (A & B) | (C & D);
                  break;
                }
                else
                {
                  // Fall-through
                }

              case ConcaveLeft: // 0.01-0.6%
                blockMask = (A & D) & (B | C);
                break;
            }

            // No pixels covered => skip block
            if (!blockMask)
            {
              continue;
            }
          }

          // Generate depth values around block
          __m256 depth0 = depth;
          __m256 depth1 = _mm256_fmadd_ps(depthDx, _mm256_set1_ps(0.5f), depth0);
          __m256 depth8 = _mm256_add_ps(depthDy, depth0);
          __m256 depth9 = _mm256_add_ps(depthDy, depth1);

          // Pack depth
          __m256i d0 = packDepthPremultiplied(depth0, depth1);
          __m256i d4 = packDepthPremultiplied(depth8, depth9);

          // Interpolate remaining values in packed space
          __m256i d2 = _mm256_avg_epu16(d0, d4);
          __m256i d1 = _mm256_avg_epu16(d0, d2);
          __m256i d3 = _mm256_avg_epu16(d2, d4);

          // Not all pixels covered - mask depth
          if (blockMask != -1)
          {
            __m128i A = _mm_cvtsi64x_si128(blockMask);
            __m128i B = _mm_slli_epi64(A, 4);
            __m256i C = _mm256_inserti128_si256(_mm256_castsi128_si256(A), B, 1);
            __m256i rowMask = _mm256_unpacklo_epi8(C, C);

            d0 = _mm256_blendv_epi8(_mm256_setzero_si256(), d0, _mm256_slli_epi16(rowMask, 3));
            d1 = _mm256_blendv_epi8(_mm256_setzero_si256(), d1, _mm256_slli_epi16(rowMask, 2));
            d2 = _mm256_blendv_epi8(_mm256_setzero_si256(), d2, _mm256_add_epi16(rowMask, rowMask));
            d3 = _mm256_blendv_epi8(_mm256_setzero_si256(), d3, rowMask);
          }

          // Test fast clear flag
          if (hiZ != 1)
          {
            // Merge depth values
            d0 = _mm256_max_epu16(_mm256_load_si256(out + 0), d0);
            d1 = _mm256_max_epu16(_mm256_load_si256(out + 1), d1);
            d2 = _mm256_max_epu16(_mm256_load_si256(out + 2), d2);
            d3 = _mm256_max_epu16(_mm256_load_si256(out + 3), d3);
          }

          // Store back new depth
          _mm256_store_si256(out + 0, d0);
          _mm256_store_si256(out + 1, d1);
          _mm256_store_si256(out + 2, d2);
          _mm256_store_si256(out + 3, d3);

          // Update HiZ
          __m256i newMinZ = _mm256_min_epu16(_mm256_min_epu16(d0, d1), _mm256_min_epu16(d2, d3));
          __m128i newMinZ16 = _mm_minpos_epu16(_mm_min_epu16(_mm256_castsi256_si128(newMinZ), _mm256_extracti128_si256(newMinZ, 1)));

          *pBlockRowHiZ = uint16_t(uint32_t(_mm_cvtsi128_si32(newMinZ16)));
        }
      }
    }
  }
}

// Force template instantiations
template void Rasterizer::rasterize<true>(const Occluder& occluder);
template void Rasterizer::rasterize<false>(const Occluder& occluder);

#endif
