#pragma once

#include <smmintrin.h>

// Cross product
inline __m128 cross(__m128 a, __m128 b)
{
  __m128 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
  __m128 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
  return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

// Normal vector of triangle
inline __m128 normal(__m128 v0, __m128 v1, __m128 v2)
{
	return cross(_mm_sub_ps(v1, v0), _mm_sub_ps(v2, v0));
}

inline __m128 normalize(__m128 v)
{
  return _mm_mul_ps(v, _mm_rsqrt_ps(_mm_dp_ps(v, v, 0x7F)));
}

struct Aabb
{
  Aabb()
  {
    m_min = _mm_set1_ps(+std::numeric_limits<float>::infinity());
    m_max = _mm_set1_ps(-std::numeric_limits<float>::infinity());
  }

  __m128 m_min, m_max;

  void include(const Aabb& aabb)
  {
    m_min = _mm_min_ps(m_min, aabb.m_min);
    m_max = _mm_max_ps(m_max, aabb.m_max);
  }

  void include(__m128 point)
  {
    m_min = _mm_min_ps(m_min, point);
    m_max = _mm_max_ps(m_max, point);
  }

  __m128 getCenter() const
  {
    return _mm_add_ps(m_min, m_max);
  }

  __m128 getExtents() const
  {
    return _mm_sub_ps(m_max, m_min);
  }

  __m128 surfaceArea()
  {
    __m128 extents = getExtents();
    __m128 extents2 = _mm_shuffle_ps(extents, extents, _MM_SHUFFLE(3, 0, 2, 1));
    return _mm_dp_ps(extents, extents2, 0x7F);
  }
};
