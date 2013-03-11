#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

EZ_FORCE_INLINE const ezVec3 ezVec2::GetAsVec3(float z)
{
  return ezVec3(x, y, z);
}

EZ_FORCE_INLINE const ezVec4 ezVec2::GetAsVec4(float z, float w)
{
  return ezVec4(x, y, z, w);
}

EZ_FORCE_INLINE const ezVec2 ezVec3::GetAsVec2() const
{
  return ezVec2(x, y);
}

EZ_FORCE_INLINE const ezVec4 ezVec3::GetAsVec4(float w) const
{
  return ezVec4(x, y, z, w);
}

EZ_FORCE_INLINE const ezVec4 ezVec3::GetAsPositionVec4() const
{
  return ezVec4(x, y, z, 1);
}

EZ_FORCE_INLINE const ezVec4 ezVec3::GetAsDirectionVec4() const
{
  return ezVec4(x, y, z, 0);
}

// *****************

EZ_FORCE_INLINE ezVec4::ezVec4()
{
#if EZ_COMPILE_FOR_DEBUG
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  x = fNaN;
  y = fNaN;
  z = fNaN;
  w = fNaN;
#endif
}

EZ_FORCE_INLINE ezVec4::ezVec4(float X, float Y, float Z, float W) : x (X), y (Y), z (Z), w (W)
{
}

EZ_FORCE_INLINE ezVec4::ezVec4(float V) : x (V), y (V), z (V), w (V)
{
}

EZ_FORCE_INLINE const ezVec2 ezVec4::GetAsVec2() const
{
  return ezVec2(x, y);
}

EZ_FORCE_INLINE const ezVec3 ezVec4::GetAsVec3() const
{
  return ezVec3(x, y, z);
}

EZ_FORCE_INLINE void ezVec4::Set(float xyzw)
{
  x = xyzw;
  y = xyzw; 
  z = xyzw; 
  w = xyzw;
}

EZ_FORCE_INLINE void ezVec4::Set(float X, float Y, float Z, float W)
{
  x = X;
  y = Y;
  z = Z;
  w = W;
}

inline void ezVec4::SetZero()
{
  x = y = z = w = 0.0f;
}

EZ_FORCE_INLINE float ezVec4::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

EZ_FORCE_INLINE float ezVec4::GetLengthSquared() const
{
  return (x * x + y * y + z * z + w * w);
}

EZ_FORCE_INLINE float ezVec4::GetLengthAndNormalize()
{
  const float fLength = GetLength();
  *this /= fLength;
  return fLength;
}

EZ_FORCE_INLINE const ezVec4 ezVec4::GetNormalized() const
{
  const float fLen = GetLength ();

  const float fLengthInv = ezMath::Invert(fLen);
  return ezVec4(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

EZ_FORCE_INLINE void ezVec4::Normalize()
{
  *this /= GetLength ();
}

inline ezResult ezVec4::NormalizeIfNotZero(const ezVec4& vFallback, float fEpsilon)
{
  const float fLength = GetLength();

  if (!ezMath::IsFinite(fLength) || ezMath::IsZero(fLength, fEpsilon))
  {
    *this = vFallback;
    return EZ_FAILURE;
  }

  *this /= fLength;
  return EZ_SUCCESS;
}

/*! \note Normalization, especially with SSE is not very precise. So this function checks whether the (squared)
  length is between a lower and upper limit.
*/
inline bool ezVec4::IsNormalized(float fEpsilon) const
{
  const float t = GetLengthSquared ();
  return ezMath::IsFloatEqual(t, 1.0f, fEpsilon);
}


inline bool ezVec4::IsZero() const
{
  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

inline bool ezVec4::IsZero(float fEpsilon) const
{
  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon) &&
          ezMath::IsZero (z, fEpsilon) &&
          ezMath::IsZero (w, fEpsilon));
}

inline bool ezVec4::IsNaN() const
{
  if (ezMath::IsNaN (x))
    return true;
  if (ezMath::IsNaN (y))
    return true;
  if (ezMath::IsNaN (z))
    return true;
  if (ezMath::IsNaN (w))
    return true;

  return false;
}

inline bool ezVec4::IsValid() const
{
  if (!ezMath::IsFinite (x))
    return false;
  if (!ezMath::IsFinite (y))
    return false;
  if (!ezMath::IsFinite (z))
    return false;
  if (!ezMath::IsFinite (w))
    return false;

  return true;
}

EZ_FORCE_INLINE const ezVec4 ezVec4::operator-() const
{
  return ezVec4(-x, -y, -z, -w);
}

EZ_FORCE_INLINE void ezVec4::operator+= (const ezVec4& cc)
{
  x += cc.x;
  y += cc.y;
  z += cc.z;
  w += cc.w;
}

EZ_FORCE_INLINE void ezVec4::operator-= (const ezVec4& cc)
{
  x -= cc.x;
  y -= cc.y;
  z -= cc.z;
  w -= cc.w;
}

EZ_FORCE_INLINE void ezVec4::operator*= (float f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;
}

EZ_FORCE_INLINE void ezVec4::operator/= (float f)
{
  const float f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
  z *= f_inv;
  w *= f_inv;
}

EZ_FORCE_INLINE float ezVec4::Dot(const ezVec4& rhs) const
{
  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

inline const ezVec4 ezVec4::CompMin(const ezVec4& rhs) const
{
  return ezVec4(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y), ezMath::Min(z, rhs.z), ezMath::Min(w, rhs.w));
}

inline const ezVec4 ezVec4::CompMax(const ezVec4& rhs) const
{
  return ezVec4(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y), ezMath::Max(z, rhs.z), ezMath::Max(w, rhs.w));
}

inline const ezVec4 ezVec4::CompMult(const ezVec4& rhs) const
{
  return ezVec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

inline const ezVec4 ezVec4::CompDiv(const ezVec4& rhs) const
{
  return ezVec4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}

EZ_FORCE_INLINE const ezVec4 operator+ (const ezVec4& v1, const ezVec4& v2)
{
  return ezVec4(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

EZ_FORCE_INLINE const ezVec4 operator- (const ezVec4& v1, const ezVec4& v2)
{
  return ezVec4(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

EZ_FORCE_INLINE const ezVec4 operator* (float f, const ezVec4& v)
{
  return ezVec4(v.x * f, v.y * f, v.z * f, v.w * f);
}

EZ_FORCE_INLINE const ezVec4 operator* (const ezVec4& v, float f)
{
  return ezVec4(v.x * f, v.y * f, v.z * f, v.w * f);
}

EZ_FORCE_INLINE const ezVec4 operator/ (const ezVec4& v, float f)
{
  // multiplication is much faster than division
  const float f_inv = ezMath::Invert(f);
  return ezVec4(v.x * f_inv, v.y * f_inv, v.z * f_inv, v.w * f_inv);
}

inline bool ezVec4::IsIdentical(const ezVec4& rhs) const
{
  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

inline bool ezVec4::IsEqual(const ezVec4& rhs, float fEpsilon) const
{
  return (ezMath::IsFloatEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsFloatEqual(y, rhs.y, fEpsilon) && 
          ezMath::IsFloatEqual(z, rhs.z, fEpsilon) &&
          ezMath::IsFloatEqual(w, rhs.w, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezVec4& v1, const ezVec4& v2)
{
  return v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator!= (const ezVec4& v1, const ezVec4& v2)
{
  return !v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator< (const ezVec4& v1, const ezVec4& v2)
{
  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;
  if (v1.z < v2.z)
    return true;
  if (v1.z > v2.z)
    return false;

  return (v1.w < v2.w);
}


