#ifndef AE_FOUNDATION_MATH_VEC3_INL
#define AE_FOUNDATION_MATH_VEC3_INL

#include "../Math.h"
#include "../../Basics/Checks.h"

namespace AE_NS_FOUNDATION
{
  inline aeVec3::aeVec3 (float X, float Y, float Z) : x (X), y (Y), z (Z)
  {
  }

  inline aeVec3::aeVec3 (float V) : x (V), y (V), z (V)
  {
  }

  inline void aeVec3::SetVector (float xyz)
  {
    x = xyz;
    y = xyz; 
    z = xyz; 
  }

  inline void aeVec3::SetVector (float X, float Y, float Z)
  {
    x = X;
    y = Y;
    z = Z;
  }

  inline void aeVec3::SetZero (void)
  {
    x = y = z = 0.0f;
  }

  inline float aeVec3::GetLength (void) const
  {
    return (aeMath::Sqrt (x * x + y * y + z * z));
  }

  inline float aeVec3::GetLengthSquared (void) const
  {
    return (x * x + y * y + z * z);
  }

  inline const aeVec3 aeVec3::GetNormalized (void) const
  {
    const float fLen = GetLength ();

    const float fLengthInv = aeMath::Invert (fLen);
    return (aeVec3 (x * fLengthInv, y * fLengthInv, z * fLengthInv));
  }

  inline void aeVec3::Normalize (void)
  {
    const float fLen = GetLength ();

    const float fLengthInv = aeMath::Invert (fLen);

    // multiplication is faster than division
    x *= fLengthInv;
    y *= fLengthInv;
    z *= fLengthInv;
  }

  inline const aeVec3 aeVec3::GetNormalizedSafe (void) const
  {
    const float fLen = GetLength ();

    if (fLen > 0.0f)
    {
      const float fLengthInv = aeMath::Invert (fLen);
      return (aeVec3 (x * fLengthInv, y * fLengthInv, z * fLengthInv));
    }
    else
      return (aeVec3 (0.0f));
  }

  inline void aeVec3::NormalizeSafe (void)
  {
    const float fLen = GetLength ();

    if (fLen > 0.0000001f)
    {
      const float fLengthInv = aeMath::Invert (fLen);

      // multiplication is faster than division
      x *= fLengthInv;
      y *= fLengthInv;
      z *= fLengthInv;
    }
    else
      x = y = z = 0.0f;
  }

  inline bool aeVec3::IsZeroVector (void) const
  {
    return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
  }

  /*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
    length is between a lower and upper limit.
  */
  inline bool aeVec3::IsNormalized (void) const
  {
    const float t = GetLengthSquared ();
    return ((t >= 0.999f) && (t <= 1.001f));
  }

  inline bool aeVec3::IsZeroVector (float fEpsilon) const
  {
    return (aeMath::IsFloatEqual (x, 0, fEpsilon) &&
            aeMath::IsFloatEqual (y, 0, fEpsilon) &&
            aeMath::IsFloatEqual (z, 0, fEpsilon));
  }

  /*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
    length is between a lower and upper limit.
  */
  inline bool aeVec3::IsNormalized (float fEpsilon) const
  {
    const float t = GetLengthSquared ();
    return aeMath::IsFloatEqual (t, 1.0f, fEpsilon);
  }

  inline bool aeVec3::IsNaN (void) const
  {
    // all comparisons fail, if a float is NaN

    if (aeMath::IsNaN (x))
      return (true);
    if (aeMath::IsNaN (y))
      return (true);
    if (aeMath::IsNaN (z))
      return (true);

    return (false);
  }

  inline bool aeVec3::IsValid () const
  {
    if (!aeMath::IsFinite (x))
      return false;
    if (!aeMath::IsFinite (y))
      return false;
    if (!aeMath::IsFinite (z))
      return false;

    return true;
  }

  inline const aeVec3 aeVec3::operator- (void) const
  {
    return (aeVec3 (-x, -y, -z));
  }

  inline const aeVec3& aeVec3::operator+= (const aeVec3& cc)
  {
    x += cc.x;
    y += cc.y;
    z += cc.z;

    return (*this);
  }

  inline const aeVec3& aeVec3::operator-= (const aeVec3& cc)
  {
    x -= cc.x;
    y -= cc.y;
    z -= cc.z;

    return (*this);
  }

  inline const aeVec3& aeVec3::operator+= (float f)
  {
    x += f;
    y += f;
    z += f;

    return (*this);
  }

  inline const aeVec3& aeVec3::operator-= (float f)
  {
    x -= f;
    y -= f;
    z -= f;

    return (*this);
  }

  inline const aeVec3& aeVec3::operator*= (float f)
  {
    x *= f;
    y *= f;
    z *= f;

    return (*this);
  }

  inline const aeVec3& aeVec3::operator/= (float f)
  {
    const float f_inv = aeMath::Invert (f);

    x *= f_inv;
    y *= f_inv;
    z *= f_inv;

    return (*this);
  }

  inline const aeVec3 aeVec3::GetAxisX (void)
  {
    return (aeVec3 (1.0f, 0.0f, 0.0f));
  }

  inline const aeVec3 aeVec3::GetAxisY (void)
  {
    return (aeVec3 (0.0f, 1.0f, 0.0f));
  }

  inline const aeVec3 aeVec3::GetAxisZ (void)
  {
    return (aeVec3 (0.0f, 0.0f, 1.0f));
  }

  inline void aeVec3::CalculateNormal (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3)
  {
    *this = (v3 - v2).Cross (v1 - v2);
    Normalize ();
  }

  inline void aeVec3::CalculateNormalSafe (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3)
  {
    *this = (v3 - v2).Cross (v1 - v2);
    NormalizeSafe ();
  }

  inline void aeVec3::MakeOrthogonalTo (const aeVec3& vNormal)
  {
    aeVec3 vOrtho = vNormal.Cross (*this);
    *this = vOrtho.Cross (vNormal);
  }

  inline const aeVec3 aeVec3::GetOrthogonalVector () const
  {
    float fDot = aeMath::Abs (this->Dot (aeVec3 (0, 1, 0)));
    if (fDot < 0.999f)
      return this->Cross (aeVec3 (0, 1, 0));
   
    return this->Cross (aeVec3 (1, 0, 0));
  }

  inline const aeVec3 aeVec3::GetReflectedVector (const aeVec3& vNormal) const 
  {
    return ((*this) - (2.0f * this->Dot (vNormal) * vNormal));
  }

  inline float aeVec3::Dot (const aeVec3& rhs) const
  {
    return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
  }

  inline const aeVec3 aeVec3::Cross (const aeVec3& rhs) const
  {
    return (aeVec3 (y * rhs.z - z * rhs.y,
                    z * rhs.x - x * rhs.z,
                    x * rhs.y - y * rhs.x));
  }

  inline float aeVec3::GetAngleBetween (const aeVec3& rhs) const
  {
    return aeMath::ACosDeg (aeMath::Clamp (this->Dot (rhs), -1.0f, 1.0f));
  }

  //inline float aeVec3::GetAngleBetweenCCW (const aeVec3& rhs, const aeVec3& vPlaneNormal) const
  //{
  //  float fAngle = this->GetAngleBetween (rhs);


  //}

  inline const aeVec3 aeVec3::CompMin (const aeVec3& rhs) const
  {
    return aeVec3 (aeMath::Min (x, rhs.x), aeMath::Min (y, rhs.y), aeMath::Min (z, rhs.z));
  }

  inline const aeVec3 aeVec3::CompMax (const aeVec3& rhs) const
  {
    return aeVec3 (aeMath::Max (x, rhs.x), aeMath::Max (y, rhs.y), aeMath::Max (z, rhs.z));
  }

  inline const aeVec3 aeVec3::CompMult (const aeVec3& rhs) const
  {
    return aeVec3 (x * rhs.x, y * rhs.y, z * rhs.z);
  }

  inline const aeVec3 aeVec3::CompDiv (const aeVec3& rhs) const
  {
    return aeVec3 (x / rhs.x, y / rhs.y, z / rhs.z);
  }

  inline const aeVec3 operator+ (const aeVec3& v1, const aeVec3& v2)
  {
    return (aeVec3 (v1.x + v2.x, v1.y + v2.y, v1.z + v2.z));
  }

  inline const aeVec3 operator- (const aeVec3& v1, const aeVec3& v2)
  {
    return (aeVec3 (v1.x - v2.x, v1.y - v2.y, v1.z - v2.z));
  }

  inline const aeVec3 operator* (float f, const aeVec3& v)
  {
    return (aeVec3 (v.x * f, v.y * f, v.z * f));
  }

  inline const aeVec3 operator+ (const aeVec3& v1, float f2) 
  { 
    return (operator+ (v1, aeVec3 (f2)));
  }

  inline const aeVec3 operator- (const aeVec3& v1, float f2) 
  { 
    return (operator- (v1, aeVec3 (f2)));
  }

  inline const aeVec3 operator* (const aeVec3& v, float f)
  {
    return (aeVec3 (v.x * f, v.y * f, v.z * f));
  }

  inline const aeVec3 operator/ (const aeVec3& v, float f)
  {
    // multiplication is much faster than division
    const float f_inv = aeMath::Invert (f);
    return (aeVec3 (v.x * f_inv, v.y * f_inv, v.z * f_inv));
  }

  //inline float operator* (const aeVec3& v1, const aeVec3& v2)
  //{
  //  return v1.Dot (v2);
  //}

  inline bool aeVec3::IsIdentical (const aeVec3& rhs) const
  {
    return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
  }

  inline bool aeVec3::IsEqual (const aeVec3& rhs, float fEpsilon) const
  {
    AE_CHECK_DEV (fEpsilon >= 0.0f, "aeVec3::IsEqual: Epsilon may not be negativ.");

    return (aeMath::IsFloatEqual (x, rhs.x, fEpsilon) && 
            aeMath::IsFloatEqual (y, rhs.y, fEpsilon) && 
            aeMath::IsFloatEqual (z, rhs.z, fEpsilon));
  }

  inline bool operator== (const aeVec3& v1, const aeVec3& v2)
  {
    return v1.IsIdentical (v2);
  }

  inline bool operator!= (const aeVec3& v1, const aeVec3& v2)
  {
    return !v1.IsIdentical (v2);
  }

  inline bool operator< (const aeVec3& v1, const aeVec3& v2)
  {
    if (v1.x < v2.x)
      return (true);
    if (v1.x > v2.x)
      return (false);
    if (v1.y < v2.y)
      return (true);
    if (v1.y > v2.y)
      return (false);

    return (v1.z < v2.z);
  }


}

#endif


