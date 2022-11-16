#pragma once

#include <memory>
#include <vector>

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
#  include <immintrin.h>
#endif

struct Occluder;

class Rasterizer
{
public:
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  Rasterizer(uint32_t width, uint32_t height);
  void setModelViewProjection(const float* matrix);
  void clear();

  template <bool possiblyNearClipped>
  void rasterize(const Occluder& occluder);

  bool queryVisibility(__m128 boundsMin, __m128 boundsMax, bool& needsClipping);

  bool query2D(uint32_t minX, uint32_t maxX, uint32_t minY, uint32_t maxY, uint32_t maxZ) const;

  void readBackDepth(void* target) const;
#else
  Rasterizer(uint32_t width, uint32_t height)
  {
  }

  void setModelViewProjection(const float* matrix) {}
  void clear() {}

  template <bool possiblyNearClipped>
  void rasterize(const Occluder& occluder)
  {
  }

  bool queryVisibility(...)
  {
    return true;
  }

  bool query2D(uint32_t minX, uint32_t maxX, uint32_t minY, uint32_t maxY, uint32_t maxZ) const
  {
    return true;
  }

  void readBackDepth(void* target) const {}
#endif

private:
#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  static float decompressFloat(uint16_t depth);

  // these functions don't always work in MSVC debug builds
  // static void transpose256(__m256 A, __m256 B, __m256 C, __m256 D, __m128* out);
  // static void transpose256i(__m256i A, __m256i B, __m256i C, __m256i D, __m128i* out);

  template <bool possiblyNearClipped>
  static void normalizeEdge(__m256& nx, __m256& ny, __m256 edgeFlipMask);

  static __m128i quantizeSlopeLookup(__m128 nx, __m128 ny);
  static __m256i quantizeSlopeLookup(__m256 nx, __m256 ny);

  static uint32_t quantizeOffsetLookup(float offset);

  static __m128i packDepthPremultiplied(__m128 depthA, __m128 depthB);
  static __m256i packDepthPremultiplied(__m256 depthA, __m256 depthB);
  static __m128i packDepthPremultiplied(__m256 depth);

  static uint64_t transposeMask(uint64_t mask);

  static void precomputeRasterizationTable();

  float m_modelViewProjection[16];
  float m_modelViewProjectionRaw[16];

  static std::vector<int64_t> m_precomputedRasterTables;
  std::vector<__m128i> m_depthBuffer;
  std::vector<uint16_t> m_hiZ;

  uint32_t m_width;
  uint32_t m_height;
  uint32_t m_blocksX;
  uint32_t m_blocksY;
#endif
};
