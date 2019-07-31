#pragma once

#include <Foundation/Math/Frustum.h>

EZ_FORCE_INLINE bool ezFrustum::Overlaps(const ezSimdBBox& object) const
{
  ezSimdVec4f center2 = object.m_Min + object.m_Max;
  ezSimdVec4f extents = object.GetExtents();

  // We're working with center and extents scaled by two - but the plane equation still works out
  // correctly since we set W = 2 here.
  center2.SetW(ezSimdFloat(2.0f));
  extents.SetW(ezSimdFloat::Zero());

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
  ezSimdVec4f minusZero;
  minusZero.Set(-0.0f);
#endif

  for (ezUInt32 plane = 0; plane < PLANE_COUNT; ++plane)
  {
    ezSimdVec4f equation;
    equation.Load<4>(m_Planes[plane].m_vNormal.GetData());

    // Change signs of extents to match signs of plane normal
    ezSimdVec4f maxExtent;

#if EZ_SIMD_IMPLEMENTATION == EZ_SIMD_IMPLEMENTATION_SSE
    // Specialized for SSE - this is faster than FlipSign for multiple calls since we can preload the constant -0.0f
    maxExtent.m_v = _mm_xor_ps(extents.m_v, _mm_andnot_ps(equation.m_v, minusZero.m_v));
#else
    maxExtent = extents.FlipSign(equation >= ezSimdVec4f::ZeroVector());
#endif

    // Compute AABB corner which is the furthest along the plane normal
    const ezSimdVec4f offset = center2 + maxExtent;

    if (equation.Dot<4>(offset) > ezSimdFloat::Zero())
    {
      // outside
      return false;
    }
  }

  // inside or intersect
  return true;
}

EZ_FORCE_INLINE bool ezFrustum::Overlaps(const ezSimdBSphere& object) const
{
  // Calculate the minimum distance of the given sphere's center to all frustum planes.
  ezSimdFloat xSphere = object.m_CenterAndRadius.x();
  ezSimdFloat ySphere = object.m_CenterAndRadius.y();
  ezSimdFloat zSphere = object.m_CenterAndRadius.z();
  ezSimdVec4f radius;
  radius.Set(object.m_CenterAndRadius.w());

  ezSimdVec4f xPlanes0;
  ezSimdVec4f yPlanes0;
  ezSimdVec4f zPlanes0;
  ezSimdVec4f dPlanes0;

  xPlanes0.Load<4>(m_Planes[0].m_vNormal.GetData());
  yPlanes0.Load<4>(m_Planes[1].m_vNormal.GetData());
  zPlanes0.Load<4>(m_Planes[2].m_vNormal.GetData());
  dPlanes0.Load<4>(m_Planes[3].m_vNormal.GetData());

  {
    ezSimdMat4f tmp;
    tmp.SetRows(xPlanes0, yPlanes0, zPlanes0, dPlanes0);
    xPlanes0 = -tmp.m_col0;
    yPlanes0 = -tmp.m_col1;
    zPlanes0 = -tmp.m_col2;
    dPlanes0 = -tmp.m_col3;
  }

  ezSimdVec4f minDist0;
  minDist0 = dPlanes0 + xPlanes0 * xSphere;
  minDist0 += yPlanes0 * ySphere;
  minDist0 += zPlanes0 * zSphere;

  ezSimdVec4f xPlanes1;
  ezSimdVec4f yPlanes1;
  ezSimdVec4f zPlanes1;
  ezSimdVec4f dPlanes1;

  xPlanes1.Load<4>(m_Planes[4].m_vNormal.GetData());
  yPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());
  zPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());
  dPlanes1.Load<4>(m_Planes[5].m_vNormal.GetData());

  {
    ezSimdMat4f tmp;
    tmp.SetRows(xPlanes1, yPlanes1, zPlanes1, dPlanes1);
    xPlanes1 = -tmp.m_col0;
    yPlanes1 = -tmp.m_col1;
    zPlanes1 = -tmp.m_col2;
    dPlanes1 = -tmp.m_col3;
  }


  ezSimdVec4f minDist1 = dPlanes1 + xPlanes1 * xSphere;
  minDist1 += yPlanes1 * ySphere;
  minDist1 += zPlanes1 * zSphere;

  ezSimdVec4f minDist = minDist0.CompMin(minDist1);

  // Add the sphere radius to cater for the sphere's extents.
  minDist += radius;

  // If the distance is still less than zero, the sphere is completely "outside" of at least one plane.
  return (minDist < ezSimdVec4f::ZeroVector()).NoneSet();
}
