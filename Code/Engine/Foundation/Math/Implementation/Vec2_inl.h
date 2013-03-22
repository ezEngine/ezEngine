#pragma once

EZ_FORCE_INLINE ezVec2::ezVec2()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  x = fNaN;
  y = fNaN;
#endif
}

EZ_FORCE_INLINE ezVec2::ezVec2(float X, float Y) : x (X), y (Y)
{
}

EZ_FORCE_INLINE ezVec2::ezVec2(float V) : x (V), y (V)
{
}

EZ_FORCE_INLINE void ezVec2::Set(float xy)
{
  x = xy;
  y = xy; 
}

EZ_FORCE_INLINE void ezVec2::Set(float X, float Y)
{
  x = X;
  y = Y;
}

EZ_FORCE_INLINE void ezVec2::SetZero()
{
  x = y = 0.0f;
}

EZ_FORCE_INLINE float ezVec2::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

EZ_FORCE_INLINE float ezVec2::GetLengthSquared() const
{
  return (x * x + y * y);
}

EZ_FORCE_INLINE float ezVec2::GetLengthAndNormalize()
{
  const float fLength = GetLength();
  *this /= fLength;
  return fLength;
}

EZ_FORCE_INLINE const ezVec2 ezVec2::GetNormalized() const
{
  const float fLen = GetLength ();

  const float fLengthInv = ezMath::Invert(fLen);
  return ezVec2(x * fLengthInv, y * fLengthInv);
}

EZ_FORCE_INLINE void ezVec2::Normalize()
{
  *this /= GetLength ();
}

inline ezResult ezVec2::NormalizeIfNotZero(const ezVec2& vFallback, float fEpsilon)
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
inline bool ezVec2::IsNormalized(float fEpsilon) const
{
  const float t = GetLengthSquared ();
  return ezMath::IsFloatEqual(t, 1.0f, fEpsilon);
}


inline bool ezVec2::IsZero() const
{
  return ((x == 0.0f) && (y == 0.0f));
}

inline bool ezVec2::IsZero(float fEpsilon) const
{
  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon));
}

inline bool ezVec2::IsNaN() const
{
  if (ezMath::IsNaN (x))
    return true;
  if (ezMath::IsNaN (y))
    return true;

  return false;
}

inline bool ezVec2::IsValid() const
{
  if (!ezMath::IsFinite (x))
    return false;
  if (!ezMath::IsFinite (y))
    return false;

  return true;
}

EZ_FORCE_INLINE const ezVec2 ezVec2::operator-() const
{
  return ezVec2(-x, -y);
}

EZ_FORCE_INLINE void ezVec2::operator+= (const ezVec2& cc)
{
  x += cc.x;
  y += cc.y;
}

EZ_FORCE_INLINE void ezVec2::operator-= (const ezVec2& cc)
{
  x -= cc.x;
  y -= cc.y;
}

EZ_FORCE_INLINE void ezVec2::operator*= (float f)
{
  x *= f;
  y *= f;
}

EZ_FORCE_INLINE void ezVec2::operator/= (float f)
{
  const float f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
}

inline void ezVec2::MakeOrthogonalTo (const ezVec2& vNormal)
{
  EZ_ASSERT(vNormal.IsNormalized (), "The normal must be normalized.");

  const float fDot = this->Dot (vNormal);
  *this -= fDot * vNormal;
}

EZ_FORCE_INLINE const ezVec2 ezVec2::GetOrthogonalVector() const
{
  EZ_ASSERT(!IsZero(ezMath_SmallEpsilon), "The vector must not be zero to be able to compute an orthogonal vector.");

  return ezVec2(-y, x);
}

inline const ezVec2 ezVec2::GetReflectedVector(const ezVec2& vNormal) const 
{
  EZ_ASSERT(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2.0f * this->Dot (vNormal) * vNormal));
}

EZ_FORCE_INLINE float ezVec2::Dot(const ezVec2& rhs) const
{
  return ((x * rhs.x) + (y * rhs.y));
}

inline float ezVec2::GetAngleBetween(const ezVec2& rhs) const
{
  EZ_ASSERT (this->IsNormalized(), "This vector must be normalized.");
  EZ_ASSERT (rhs.IsNormalized(), "The other vector must be normalized.");

  return ezMath::ACosDeg (ezMath::Clamp(this->Dot (rhs), -1.0f, 1.0f));
}

EZ_FORCE_INLINE const ezVec2 ezVec2::CompMin(const ezVec2& rhs) const
{
  return ezVec2(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y));
}

EZ_FORCE_INLINE const ezVec2 ezVec2::CompMax(const ezVec2& rhs) const
{
  return ezVec2(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y));
}

EZ_FORCE_INLINE const ezVec2 ezVec2::CompMult(const ezVec2& rhs) const
{
  return ezVec2(x * rhs.x, y * rhs.y);
}

EZ_FORCE_INLINE const ezVec2 ezVec2::CompDiv(const ezVec2& rhs) const
{
  return ezVec2(x / rhs.x, y / rhs.y);
}

EZ_FORCE_INLINE const ezVec2 operator+ (const ezVec2& v1, const ezVec2& v2)
{
  return ezVec2(v1.x + v2.x, v1.y + v2.y);
}

EZ_FORCE_INLINE const ezVec2 operator- (const ezVec2& v1, const ezVec2& v2)
{
  return ezVec2(v1.x - v2.x, v1.y - v2.y);
}

EZ_FORCE_INLINE const ezVec2 operator* (float f, const ezVec2& v)
{
  return ezVec2(v.x * f, v.y * f);
}

EZ_FORCE_INLINE const ezVec2 operator* (const ezVec2& v, float f)
{
  return ezVec2(v.x * f, v.y * f);
}

EZ_FORCE_INLINE const ezVec2 operator/ (const ezVec2& v, float f)
{
  // multiplication is much faster than division
  const float f_inv = ezMath::Invert(f);
  return ezVec2(v.x * f_inv, v.y * f_inv);
}

inline bool ezVec2::IsIdentical(const ezVec2& rhs) const
{
  return ((x == rhs.x) && (y == rhs.y));
}

inline bool ezVec2::IsEqual(const ezVec2& rhs, float fEpsilon) const
{
  return (ezMath::IsFloatEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsFloatEqual(y, rhs.y, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezVec2& v1, const ezVec2& v2)
{
  return v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator!= (const ezVec2& v1, const ezVec2& v2)
{
  return !v1.IsIdentical(v2);
}

EZ_FORCE_INLINE bool operator< (const ezVec2& v1, const ezVec2& v2)
{
  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;

  return (v1.y < v2.y);
}


