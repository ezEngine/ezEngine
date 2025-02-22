#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/SimdMath/SimdVec4i.h>

namespace ezSimdConversion
{
  EZ_ALWAYS_INLINE ezVec3 ToVec3(const ezSimdVec4f& v)
  {
    ezVec4 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<ezVec3*>(&tmp.x);
  }

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec3(const ezVec3& v)
  {
    ezSimdVec4f tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezVec3I32 ToVec3i(const ezSimdVec4i& v)
  {
    ezVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return *reinterpret_cast<ezVec3I32*>(&tmp.x);
  }

  EZ_ALWAYS_INLINE ezSimdVec4i ToVec3i(const ezVec3I32& v)
  {
    ezSimdVec4i tmp;
    tmp.Load<3>(&v.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezVec4 ToVec4(const ezSimdVec4f& v)
  {
    ezVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec4(const ezVec4& v)
  {
    ezSimdVec4f tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezVec4I32 ToVec4i(const ezSimdVec4i& v)
  {
    ezVec4I32 tmp;
    v.Store<4>(&tmp.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezSimdVec4i ToVec4i(const ezVec4I32& v)
  {
    ezSimdVec4i tmp;
    tmp.Load<4>(&v.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezQuat ToQuat(const ezSimdQuat& q)
  {
    ezQuat tmp;
    q.m_v.Store<4>(&tmp.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezSimdQuat ToQuat(const ezQuat& q)
  {
    ezSimdVec4f tmp;
    tmp.Load<4>(&q.x);
    return ezSimdQuat(tmp);
  }

  EZ_ALWAYS_INLINE ezTransform ToTransform(const ezSimdTransform& t)
  {
    return ezTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline ezSimdTransform ToTransform(const ezTransform& t)
  {
    return ezSimdTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation), ToVec3(t.m_vScale));
  }

  EZ_ALWAYS_INLINE ezMat4 ToMat4(const ezSimdMat4f& m)
  {
    ezMat4 tmp;
    m.GetAsArray(tmp.m_fElementsCM, ezMatrixLayout::ColumnMajor);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezSimdMat4f ToMat4(const ezMat4& m)
  {
    return ezSimdMat4f::MakeFromColumnMajorArray(m.m_fElementsCM);
  }

  EZ_ALWAYS_INLINE ezBoundingBoxSphere ToBBoxSphere(const ezSimdBBoxSphere& b)
  {
    ezVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return ezBoundingBoxSphere::MakeFromCenterExtents(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  EZ_ALWAYS_INLINE ezSimdBBoxSphere ToBBoxSphere(const ezBoundingBoxSphere& b)
  {
    return ezSimdBBoxSphere::MakeFromCenterExtents(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtents), b.m_fSphereRadius);
  }

  EZ_ALWAYS_INLINE ezBoundingSphere ToBSphere(const ezSimdBSphere& s)
  {
    ezVec4 centerAndRadius = ToVec4(s.m_CenterAndRadius);
    return ezBoundingSphere::MakeFromCenterAndRadius(centerAndRadius.GetAsVec3(), centerAndRadius.w);
  }

  EZ_ALWAYS_INLINE ezSimdBSphere ToBSphere(const ezBoundingSphere& s)
  {
    return ezSimdBSphere(ToVec3(s.m_vCenter), s.m_fRadius);
  }

  EZ_ALWAYS_INLINE ezSimdBBox ToBBox(const ezBoundingBox& b)
  {
    return ezSimdBBox(ToVec3(b.m_vMin), ToVec3(b.m_vMax));
  }

  EZ_ALWAYS_INLINE ezBoundingBox ToBBox(const ezSimdBBox& b)
  {
    return ezBoundingBox::MakeFromMinMax(ToVec3(b.m_Min), ToVec3(b.m_Max));
  }

}; // namespace ezSimdConversion
