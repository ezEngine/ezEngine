#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Mat4.h>

EZ_FORCE_INLINE ezQuat::ezQuat()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  w = fNaN;
#endif
}

EZ_FORCE_INLINE ezQuat::ezQuat(float X, float Y, float Z, float W) : v(X,Y,Z), w(W)
{
}

EZ_FORCE_INLINE void ezQuat::SetElements(float X, float Y, float Z, float W)
{
  v.Set (X, Y, Z);
  w = W;
}

EZ_FORCE_INLINE void ezQuat::SetIdentity()
{
  v.SetZero ();
  w = 1.0f;
}

inline void ezQuat::SetFromAxisAndAngle(const ezVec3& vRotationAxis, float fAngle)
{
  const float d = fAngle * ezMath_DegToRad * 0.5f;

  v = ezMath::SinRad (d) * vRotationAxis;
  w = ezMath::CosRad (d);

  Normalize ();
}

inline void ezQuat::Normalize()
{
  float n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;
    
  n = ezMath::Invert(ezMath::Sqrt(n));

  v *= n;
  w *= n;
}

inline ezResult ezQuat::GetRotationAxisAndAngle(ezVec3& vAxis, float& fAngle) const
{
  const float acos = ezMath::ACosRad(w);
  const float d = ezMath::SinRad(acos);

  if (d == 0.0f)
    return EZ_FAILURE;

  vAxis = (v / d);
  fAngle = ezMath::RadToDeg(acos * 2.0f);

  return EZ_SUCCESS;
}

EZ_FORCE_INLINE const ezQuat ezQuat::operator-() const
{
  return (ezQuat (-v.x, -v.y, -v.z, w));
}

inline const ezVec3 operator* (const ezQuat& q, const ezVec3& v)
{
  ezQuat qt (v.x, v.y, v.z, 0.0f);
  return (((q * qt) * (-q)).v);
}

EZ_FORCE_INLINE const ezQuat operator* (const ezQuat& q1, const ezQuat& q2)
{
  ezQuat q;

  q.w = q1.w * q2.w - q1.v.Dot (q2.v);
  q.v = q1.w * q2.v + q2.w * q1.v + q1.v.Cross (q2.v);

  return (q);
}

inline bool ezQuat::IsValid(float fEpsilon) const
{
  if (!v.IsValid())
    return false;
  if (!ezMath::IsFinite(w))
    return false;

  float n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;
    
  return (ezMath::IsFloatEqual(n, 1.0f, fEpsilon));
}

inline bool ezQuat::IsEqualRotation(const ezQuat& qOther, float fEpsilon) const
{
  ezVec3 vA1, vA2;
  float fA1, fA2;

  if (GetRotationAxisAndAngle(vA1, fA1) == EZ_FAILURE)
    return false;
  if (qOther.GetRotationAxisAndAngle(vA2, fA2) == EZ_FAILURE)
    return false;

  if ((ezMath::IsFloatEqual(fA1, fA2, fEpsilon)) &&
      (vA1.IsEqual(vA2, fEpsilon)))
      return true;

  if ((ezMath::IsFloatEqual(fA1, -fA2, fEpsilon)) &&
      (vA1.IsEqual(-vA2, fEpsilon)))
      return true;

  return false;
}


