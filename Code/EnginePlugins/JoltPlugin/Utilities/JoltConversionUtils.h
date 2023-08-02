#pragma once

#include <Foundation/SimdMath/SimdTransform.h>
#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Vec4.h>

namespace ezJoltConversionUtils
{
  EZ_ALWAYS_INLINE ezVec3 ToVec3(const JPH::Vec3& v) { return ezVec3(v.mF32[0], v.mF32[1], v.mF32[2]); }

  EZ_ALWAYS_INLINE ezVec3 ToVec3(const JPH::Float3& v) { return reinterpret_cast<const ezVec3&>(v); }

  EZ_ALWAYS_INLINE ezColor ToColor(const JPH::ColorArg& c)
  {
    const JPH::Vec4 v4 = c.ToVec4();
    return reinterpret_cast<const ezColor&>(v4);
  }

  EZ_ALWAYS_INLINE ezSimdVec4f ToSimdVec3(const JPH::Vec3& v) { return ezSimdVec4f(v.mF32[0], v.mF32[1], v.mF32[2], v.mF32[3]); }

  EZ_ALWAYS_INLINE JPH::Vec3 ToVec3(const ezVec3& v) { return JPH::Vec3(v.x, v.y, v.z); }

  EZ_ALWAYS_INLINE JPH::Float3 ToFloat3(const ezVec3& v) { return reinterpret_cast<const JPH::Float3&>(v); }

  EZ_ALWAYS_INLINE JPH::Vec3 ToVec3(const ezSimdVec4f& v) { return reinterpret_cast<const JPH::Vec3&>(v); }

  EZ_ALWAYS_INLINE ezQuat ToQuat(const JPH::Quat& q) { return reinterpret_cast<const ezQuat&>(q); }

  EZ_ALWAYS_INLINE ezSimdQuat ToSimdQuat(const JPH::Quat& q) { return reinterpret_cast<const ezSimdQuat&>(q); }

  EZ_ALWAYS_INLINE JPH::Quat ToQuat(const ezQuat& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }

  EZ_ALWAYS_INLINE JPH::Quat ToQuat(const ezSimdQuat& q) { return reinterpret_cast<const JPH::Quat&>(q); }

  EZ_ALWAYS_INLINE ezTransform ToTransform(const JPH::Vec3& pos, const JPH::Quat& rot) { return ezTransform(ToVec3(pos), ToQuat(rot)); }

  EZ_ALWAYS_INLINE ezTransform ToTransform(const JPH::Vec3& pos) { return ezTransform(ToVec3(pos)); }

} // namespace ezJoltConversionUtils
