#pragma once

#include <PhysXPlugin/Basics.h>
#include <Foundation/SimdMath/SimdTransform.h>

#include <PxPhysicsAPI.h>

namespace ezPxConversionUtils
{
  EZ_ALWAYS_INLINE ezVec3 ToVec3(const physx::PxVec3& v)
  {
    return ezVec3(v.x, v.y, v.z);
  }

  EZ_ALWAYS_INLINE ezSimdVec4f ToSimdVec3(const physx::PxVec3& v)
  {
    return ezSimdVec4f(v.x, v.y, v.z);
  }

  EZ_ALWAYS_INLINE physx::PxVec3 ToVec3(const ezVec3& v)
  {
    return physx::PxVec3(v.x, v.y, v.z);
  }

  EZ_ALWAYS_INLINE physx::PxVec3 ToVec3(const ezSimdVec4f& v)
  {
    ezVec4 tmp;
    v.Store<4>(&tmp.x);
    return ToVec3(tmp.GetAsVec3());
  }

  EZ_ALWAYS_INLINE ezVec3 ToVec3(const physx::PxExtendedVec3& v)
  {
    physx::PxVec3 fVec = toVec3(v);
    return ezVec3(fVec.x, fVec.y, fVec.z);
  }

  EZ_ALWAYS_INLINE physx::PxExtendedVec3 ToExVec3(const ezVec3& v)
  {
    return physx::PxExtendedVec3(v.x, v.y, v.z);
  }

  EZ_ALWAYS_INLINE ezQuat ToQuat(const physx::PxQuat& q)
  {
    return ezQuat(q.x, q.y, q.z, q.w);
  }

  EZ_ALWAYS_INLINE ezSimdQuat ToSimdQuat(const physx::PxQuat& q)
  {
    return ezSimdQuat(ezSimdVec4f(q.x, q.y, q.z, q.w));
  }

  EZ_ALWAYS_INLINE physx::PxQuat ToQuat(const ezQuat& q)
  {
    return physx::PxQuat(q.v.x, q.v.y, q.v.z, q.w);
  }

  EZ_ALWAYS_INLINE physx::PxQuat ToQuat(const ezSimdQuat& q)
  {
    ezQuat tmp;
    q.m_v.Store<4>(&tmp.v.x);
    return ToQuat(tmp);
  }

  EZ_ALWAYS_INLINE ezTransform ToTransform(const physx::PxTransform& t)
  {
    return ezTransform(ToVec3(t.p), ToQuat(t.q));
  }

  EZ_ALWAYS_INLINE ezSimdTransform ToSimdTransform(const physx::PxTransform& t)
  {
    return ezSimdTransform(ToSimdVec3(t.p), ToSimdQuat(t.q));
  }

  EZ_ALWAYS_INLINE physx::PxTransform ToTransform(const ezTransform& t)
  {
    return physx::PxTransform(ToVec3(t.m_vPosition), ToQuat(t.m_qRotation));
  }

  EZ_ALWAYS_INLINE physx::PxTransform ToTransform(const ezSimdTransform& t)
  {
    return physx::PxTransform(ToVec3(t.m_Position), ToQuat(t.m_Rotation));
  }

  EZ_ALWAYS_INLINE physx::PxTransform ToTransform(const ezVec3& p, const ezQuat& q)
  {
    return physx::PxTransform(ToVec3(p), ToQuat(q));
  }

  EZ_ALWAYS_INLINE physx::PxMeshScale ToScale(const ezSimdTransform& t)
  {
    return physx::PxMeshScale(ToVec3(t.m_Scale), physx::PxQuat(physx::PxIdentity));
  }
}

