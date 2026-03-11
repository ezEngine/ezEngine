#pragma once

#include <memory>
#include <vector>

#include <Foundation/Basics.h>

#if EZ_ENABLED(EZ_COMPILER_MSVC_PURE) && EZ_ENABLED(EZ_PLATFORM_64BIT)
#  define EZ_RASTERIZER_SUPPORTED EZ_ON
#else
#  define EZ_RASTERIZER_SUPPORTED EZ_OFF
#endif

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
#  include <intrin.h>
#endif

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)

struct Occluder
{
  ~Occluder();

  void bake(const __m128* vertices, ezUInt32 numVertices, __m128 refMin, __m128 refMax);

  __m128 m_center;

  __m128 m_refMin;
  __m128 m_refMax;

  __m128 m_boundsMin;
  __m128 m_boundsMax;

  __m256i* m_vertexData = nullptr;
  uint32_t m_packetCount;
};
#else

struct Occluder
{
};

#endif
