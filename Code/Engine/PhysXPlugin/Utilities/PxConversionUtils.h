#pragma once

#include <PhysXPlugin/Basics.h>

#include <PxPhysicsAPI.h>

namespace ezPxConversionUtils
{
  EZ_FORCE_INLINE ezVec3 ToVec3(const physx::PxVec3& v)
  {
    return ezVec3(v.x, v.y, v.z);
  }

  EZ_FORCE_INLINE physx::PxVec3 ToVec3(const ezVec3& v)
  {
    return physx::PxVec3(v.x, v.y, v.z);
  }

  EZ_FORCE_INLINE ezVec3 ToVec3(const physx::PxExtendedVec3& v)
  {
    physx::PxVec3 fVec = toVec3(v);
    return ezVec3(fVec.x, fVec.y, fVec.z);
  }

  EZ_FORCE_INLINE physx::PxExtendedVec3 ToExVec3(const ezVec3& v)
  {
    return physx::PxExtendedVec3(v.x, v.y, v.z);
  }

  EZ_FORCE_INLINE ezQuat ToQuat(const physx::PxQuat& q)
  {
    return ezQuat(q.x, q.y, q.z, q.w);
  }

  EZ_FORCE_INLINE physx::PxQuat ToQuat(const ezQuat& q)
  {
    return physx::PxQuat(q.v.x, q.v.y, q.v.z, q.w);
  }

  EZ_FORCE_INLINE ezTransform ToTransform(const physx::PxTransform& t)
  {
    return ezTransform(ToVec3(t.p), ToQuat(t.q));
  }

  EZ_FORCE_INLINE physx::PxTransform ToTransform(const ezVec3& p, const ezQuat& q)
  {
    return physx::PxTransform(ToVec3(p), ToQuat(q));
  }
}

