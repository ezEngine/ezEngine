#ifndef AE_FOUNDATION_MATH_QUATERNION_INL
#define AE_FOUNDATION_MATH_QUATERNION_INL

#include "../Vec3.h"
#include "../Matrix.h"

namespace AE_NS_FOUNDATION
{
  inline aeQuaternion::aeQuaternion (Initialization Init)
  {
    switch (Init)
    {
    case Identity:
      SetIdentity ();
      return;
    default:
      AE_CHECK_DEV (false, "Unknown initialization type %i.", Init);
      return;
    }
  }

  inline aeQuaternion::aeQuaternion (float X, float Y, float Z, float W) : v (X, Y, Z), w (W)
  {
  }

  inline aeQuaternion::aeQuaternion (const aeVec3& V, float W) : v (V), w (W)
  {
  }

  inline void aeQuaternion::SetQuaternion (const aeVec3& V, float W)
  {
    v = V;
    w = W;
  }

  inline void aeQuaternion::SetQuaternion (float X, float Y, float Z, float W)
  {
    v.SetVector (X, Y, Z);
    w = W;
  }

  inline void aeQuaternion::SetIdentity (void)
  {
    v.SetZero ();
    w = 1.0f;
  }

  inline void aeQuaternion::CreateQuaternion (const aeVec3& vRotationAxis, float fAngle)
  {
    const float d = fAngle * aeMath_DegToRad * 0.5f;

    v = aeMath::SinRad (d) * vRotationAxis;
    w = aeMath::CosRad (d);

    Normalize ();
  }

  inline void aeQuaternion::Normalize (void)
  {
    float n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;
    
    if (n == 1.0f)
      return;

    n = aeMath::Invert (aeMath::Sqrt (n));

    v *= n;
    w *= n;
  }

  inline const aeQuaternion aeQuaternion::GetNormalized (void) const
  {
    float n = v.x * v.x + v.y * v.y + v.z * v.z + w * w;
    
    if (n == 1.0f)
      return (*this);

    n = aeMath::Invert (aeMath::Sqrt (n));

    return (aeQuaternion (v.x * n, v.y * n, v.z * n, w * n));
  }

  inline const aeVec3 aeQuaternion::GetRotationAxis (void) const
  {
    const float d = aeMath::SinRad (aeMath::ACosRad (w));

    if (d == 0.0f)
      return (aeVec3 (0, 1, 0));

    return (v / d);
  }

  inline float aeQuaternion::GetRotationAngle (void) const
  {
    return (aeMath::ACosDeg (w) * 2.0f);
  }

  inline const aeQuaternion aeQuaternion::operator- (void) const
  {
    return (aeQuaternion (-v.x, -v.y, -v.z, w));
  }

  inline const aeVec3 operator* (const aeQuaternion& q, const aeVec3& v)
  {
    aeQuaternion qt (v.x, v.y, v.z, 0.0f);
    return (((q * qt) * (-q)).v);
  }

  inline const aeQuaternion operator* (const aeQuaternion& q1, const aeQuaternion& q2)
  {
    aeQuaternion q;

    q.w = q1.w * q2.w - q1.v.Dot (q2.v);
    q.v = q1.w * q2.v + q2.w * q1.v + q1.v.Cross (q2.v);

    return (q);
  }
}

#endif


