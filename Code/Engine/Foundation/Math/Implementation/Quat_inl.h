#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Mat4.h>

inline ezQuat::ezQuat(float X, float Y, float Z, float W)
{
  SetElements(X, Y, Z, W);
}

EZ_FORCE_INLINE void ezQuat::SetElements (const ezVec3& V, float W)
{
  v = V;
  w = W;
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
    
  if (n == 1.0f)
    return;

  n = ezMath::Invert (ezMath::Sqrt (n));

  v *= n;
  w *= n;
}

inline const ezQuat ezQuat::GetNormalized() const
{
  float n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;
    
  if (n == 1.0f)
    return (*this);

  n = ezMath::Invert (ezMath::Sqrt (n));

  return (ezQuat(v.x * n, v.y * n, v.z * n, w * n));
}

inline const ezVec3 ezQuat::GetRotationAxis() const
{
  const float d = ezMath::SinRad (ezMath::ACosRad (w));

  if (d == 0.0f)
    return (ezVec3 (0, 1, 0));

  return (v / d);
}

inline float ezQuat::GetRotationAngle() const
{
  return (ezMath::ACosDeg (w) * 2.0f);
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

inline bool ezQuat::IsIdentical(const ezQuat& rhs) const
{
  return (v.IsIdentical(rhs.v) && (w == rhs.w));
}

inline bool ezQuat::IsEqual(const ezQuat& rhs, float fEpsilon) const
{
  return (v.IsEqual(rhs.v, fEpsilon) && ezMath::IsFloatEqual(w, rhs.w, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezQuat& q1, const ezQuat& q2)
{
  return q1.IsIdentical(q2);
}

EZ_FORCE_INLINE bool operator!= (const ezQuat& q1, const ezQuat& q2)
{
  return !q1.IsIdentical(q2);
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




