#pragma once

#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>

namespace ezSimdConversion
{
  EZ_ALWAYS_INLINE ezVec3 ToVec3(const ezSimdVec4f& v)
  {
    ezVec4 tmp;
    v.Store<4>(&tmp.x);
    return tmp.GetAsVec3();
  }

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec3(const ezVec3& v)
  {
    ezSimdVec4f tmp;
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

  EZ_ALWAYS_INLINE ezQuat ToQuat(const ezSimdQuat& q)
  {
    ezQuat tmp;
    q.m_v.Store<4>(&tmp.v.x);
    return tmp;
  }

  EZ_ALWAYS_INLINE ezSimdQuat ToQuat(const ezQuat& q)
  {
    ezSimdVec4f tmp;
    tmp.Load<4>(&q.v.x);
    return ezSimdQuat(tmp);
  }

  EZ_ALWAYS_INLINE ezTransform ToTransform(const ezSimdTransform& t)
  {
    return ezTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation), ToVec3(t.m_Scale));
  }

  inline ezSimdTransform ToTransform(const ezTransform& t)
  {
    ezVec3 position; ezQuat rotation; ezVec3 scale;
    t.Decompose(position, rotation, scale);

    return ezSimdTransform(ToVec3(position), ToQuat(rotation), ToVec3(scale));
  }

  EZ_ALWAYS_INLINE ezBoundingBoxSphere ToBBoxSphere(const ezSimdBBoxSphere& b)
  {
    ezVec4 centerAndRadius = ToVec4(b.m_CenterAndRadius);
    return ezBoundingBoxSphere(centerAndRadius.GetAsVec3(), ToVec3(b.m_BoxHalfExtents), centerAndRadius.w);
  }

  EZ_ALWAYS_INLINE ezSimdBBoxSphere ToBBoxSphere(const ezBoundingBoxSphere& b)
  {
    return ezSimdBBoxSphere(ToVec3(b.m_vCenter), ToVec3(b.m_vBoxHalfExtends), b.m_fSphereRadius);
  }
};
