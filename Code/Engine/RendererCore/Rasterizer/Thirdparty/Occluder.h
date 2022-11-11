#pragma once

#include <intrin.h>
#include <memory>
#include <vector>

struct Occluder
{
  void bake(const __m128* vertices, size_t numVertices, __m128 refMin, __m128 refMax);

  __m128 m_center;

  __m128 m_refMin;
  __m128 m_refMax;

  __m128 m_boundsMin;
  __m128 m_boundsMax;

  __m256i* m_vertexData = nullptr;
  uint32_t m_packetCount;
};
