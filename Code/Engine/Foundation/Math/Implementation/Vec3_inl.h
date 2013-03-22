#pragma once

EZ_FORCE_INLINE ezVec3::ezVec3()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  x = fNaN;
  y = fNaN;
  z = fNaN;
#endif
}

EZ_FORCE_INLINE ezVec3::ezVec3(float X, float Y, float Z) : x (X), y (Y), z (Z)
{
}

EZ_FORCE_INLINE ezVec3::ezVec3(float V) : x (V), y (V), z (V)
{
}

EZ_FORCE_INLINE void ezVec3::Set(float xyz)
{
  x = xyz;
  y = xyz; 
  z = xyz; 
}

EZ_FORCE_INLINE void ezVec3::Set(float X, float Y, float Z)
{
  x = X;
  y = Y;
  z = Z;
}

EZ_FORCE_INLINE void ezVec3::SetZero()
{
  x = y = z = 0.0f;
}

EZ_FORCE_INLINE float ezVec3::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

inline ezResult ezVec3::SetLength(float fNewLength, float fEpsilon /* = ezMath_DefaultEpsilon */)
{
  if (NormalizeIfNotZero(ezVec3::ZeroVector(), fEpsilon) == EZ_FAILURE)
    return EZ_FAILURE;

  *this *= fNewLength;
  return EZ_SUCCESS;
}

EZ_FORCE_INLINE float ezVec3::GetLengthSquared() const
{
  return (x * x + y * y + z * z);
}

EZ_FORCE_INLINE float ezVec3::GetLengthAndNormalize()
{
  const float fLength = GetLength();
  *this /= fLength;
  return fLength;
}

EZ_FORCE_INLINE const ezVec3 ezVec3::GetNormalized() const
{
  const float fLen = GetLength ();

  const float fLengthInv = ezMath::Invert(fLen);
  return ezVec3(x * fLengthInv, y * fLengthInv, z * fLengthInv);
}

EZ_FORCE_INLINE void ezVec3::Normalize()
{
  *this /= GetLength ();
}

inline ezResult ezVec3::NormalizeIfNotZero(const ezVec3& vFallback, float fEpsilon)
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
EZ_FORCE_INLINE bool ezVec3::IsNormalized(float fEpsilon) const
{
  const float t = GetLengthSquared ();
  return ezMath::IsFloatEqual(t, 1.0f, fEpsilon);
}


EZ_FORCE_INLINE bool ezVec3::IsZero() const
{
  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
}

inline bool ezVec3::IsZero(float fEpsilon) const
{
  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon) &&
          ezMath::IsZero (z, fEpsilon));
}

inline bool ezVec3::IsNaN() const
{
  if (ezMath::IsNaN(x))
    return true;
  if (ezMath::IsNaN(y))
    return true;
  if (ezMath::IsNaN(z))
    return true;

  return false;
}

inline bool ezVec3::IsValid() const
{
  if (!ezMath::IsFinite(x))
    return false;
  if (!ezMath::IsFinite(y))
    return false;
  if (!ezMath::IsFinite(z))
    return false;

  return true;
}

EZ_FORCE_INLINE const ezVec3 ezVec3::operator-() const
{
  return ezVec3(-x, -y, -z);
}

EZ_FORCE_INLINE void ezVec3::operator+= (const ezVec3& cc)
{
  x += cc.x;
  y += cc.y;
  z += cc.z;
}

EZ_FORCE_INLINE void ezVec3::operator-= (const ezVec3& cc)
{
  x -= cc.x;
  y -= cc.y;
  z -= cc.z;
}

EZ_FORCE_INLINE void ezVec3::operator*= (float f)
{
  x *= f;
  y *= f;
  z *= f;
}

EZ_FORCE_INLINE void ezVec3::operator/= (float f)
{
  const float f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
  z *= f_inv;
}

inline ezResult ezVec3::CalculateNormal(const ezVec3& v1, const ezVec3& v2, const ezVec3& v3)
{
  *this = (v3 - v2).Cross(v1 - v2);
  return NormalizeIfNotZero();
}

inline void ezVec3::MakeOrthogonalTo(const ezVec3& vNormal)
{
  EZ_ASSERT(vNormal.IsNormalized(), "The vector to make this vector orthogonal to, must be normalized. It's length is %.3f", vNormal.GetLength());

  ezVec3 vOrtho = vNormal.Cross(*this);
  *this = vOrtho.Cross(vNormal);
}

inline const ezVec3 ezVec3::GetOrthogonalVector() const
{
  EZ_ASSERT(!IsZero(ezMath_SmallEpsilon), "The vector must not be zero to be able to compute an orthogonal vector.");

  float fDot = ezMath::Abs(this->Dot (ezVec3 (0, 1, 0)));
  if (fDot < 0.999f)
    return this->Cross(ezVec3 (0, 1, 0));
   
  return this->Cross(ezVec3 (1, 0, 0));
}

inline const ezVec3 ezVec3::GetReflectedVector(const ezVec3& vNormal) const 
{
  EZ_ASSERT(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2.0f * this->Dot (vNormal) * vNormal));
}

EZ_FORCE_INLINE float ezVec3::Dot(const ezVec3& rhs) const
{
  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
}

inline const ezVec3 ezVec3::Cross(const ezVec3& rhs) const
{
  return ezVec3(y * rhs.z - z * rhs.y,
                z * rhs.x - x * rhs.z,
                x * rhs.y - y * rhs.x);
}

inline float ezVec3::GetAngleBetween(const ezVec3& rhs) const
{
  EZ_ASSERT (this->IsNormalized(), "This vector must be normalized.");
  EZ_ASSERT (rhs.IsNormalized(), "The other vector must be normalized.");

  return ezMath::ACosDeg (ezMath::Clamp(this->Dot (rhs), -1.0f, 1.0f));
}

EZ_FORCE_INLINE const ezVec3 ezVec3::CompMin(const ezVec3& rhs) const
{
  return ezVec3(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y), ezMath::Min(z, rhs.z));
}

EZ_FORCE_INLINE const ezVec3 ezVec3::CompMax(const ezVec3& rhs) const
{
  return ezVec3(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y), ezMath::Max(z, rhs.z));
}

EZ_FORCE_INLINE const ezVec3 ezVec3::CompMult(const ezVec3& rhs) const
{
  return ezVec3(x * rhs.x, y * rhs.y, z * rhs.z);
}

EZ_FORCE_INLINE const ezVec3 ezVec3::CompDiv(const ezVec3& rhs) const
{
  return ezVec3(x / rhs.x, y / rhs.y, z / rhs.z);
}

EZ_FORCE_INLINE const ezVec3 operator+ (const ezVec3& v1, const ezVec3& v2)
{
  return ezVec3(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

EZ_FORCE_INLINE const ezVec3 operator- (const ezVec3& v1, const ezVec3& v2)
{
  return ezVec3(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

EZ_FORCE_INLINE const ezVec3 operator* (float f, const ezVec3& v)
{
  return ezVec3(v.x * f, v.y * f, v.z * f);
}

EZ_FORCE_INLINE const ezVec3 operator* (const ezVec3& v, float f)
{
  return ezVec3(v.x * f, v.y * f, v.z * f);
}

EZ_FORCE_INLINE const ezVec3 operator/ (const ezVec3& v, float f)
{
  // multiplication is much faster than division
  const float f_inv = ezMath::Invert(f);
  return ezVec3(v.x * f_inv, v.y * f_inv, v.z * f_inv);
}

inline bool ezVec3::IsIdentical(const ezVec3& rhs) const
{
  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

inline bool ezVec3::IsEqual(const ezVec3& rhs, float fEpsilon) const
{
  return (ezMath::IsFloatEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsFloatEqual(y, rhs.y, fEpsilon) && 
          ezMath::IsFloatEqual(z, rhs.z, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezVec3& v1, const ezVec3& v2)
{
  return v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator!= (const ezVec3& v1, const ezVec3& v2)
{
  return !v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator< (const ezVec3& v1, const ezVec3& v2)
{
  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;
  if (v1.y < v2.y)
    return true;
  if (v1.y > v2.y)
    return false;

  return (v1.z < v2.z);
}


