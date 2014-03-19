#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec2Template<Type>::GetAsVec3(Type z) const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(x, y, z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec2Template<Type>::GetAsVec4(Type z, Type w) const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, w);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec3Template<Type>::GetAsVec2() const
{
  EZ_NAN_ASSERT(this);

  return ezVec2Template<Type>(x, y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsVec4(Type w) const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, w);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsPositionVec4() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, 1);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsDirectionVec4() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, 0);
}

// *****************

template<typename Type>
EZ_FORCE_INLINE ezVec4Template<Type>::ezVec4Template()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
  w = TypeNaN;
#endif
}

template<typename Type>
EZ_FORCE_INLINE ezVec4Template<Type>::ezVec4Template(Type X, Type Y, Type Z, Type W) : x (X), y (Y), z (Z), w (W)
{
}

template<typename Type>
EZ_FORCE_INLINE ezVec4Template<Type>::ezVec4Template(Type V) : x (V), y (V), z (V), w (V)
{
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec4Template<Type>::GetAsVec2() const
{
  EZ_NAN_ASSERT(this);

  return ezVec2Template<Type>(x, y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec4Template<Type>::GetAsVec3() const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(x, y, z);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::Set(Type xyzw)
{
  x = xyzw;
  y = xyzw; 
  z = xyzw; 
  w = xyzw;
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::Set(Type X, Type Y, Type Z, Type W)
{
  x = X;
  y = Y;
  z = Z;
  w = W;
}

template<typename Type>
inline void ezVec4Template<Type>::SetZero()
{
  x = y = z = w = 0.0f;
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::GetLengthSquared() const
{
  EZ_NAN_ASSERT(this);

  return (x * x + y * y + z * z + w * w);
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec4Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength ();

  const Type fLengthInv = ezMath::Invert(fLen);
  return ezVec4Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::Normalize()
{
  *this /= GetLength ();
}

template<typename Type>
inline ezResult ezVec4Template<Type>::NormalizeIfNotZero(const ezVec4Template<Type>& vFallback, Type fEpsilon)
{
  EZ_NAN_ASSERT(&vFallback);

  const Type fLength = GetLength();

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
template<typename Type>
inline bool ezVec4Template<Type>::IsNormalized(Type fEpsilon /* = ezMath::BasicType<Type>::HugeEpsilon() */) const
{
  const Type t = GetLengthSquared ();
  return ezMath::IsEqual(t, (Type) 1, fEpsilon);
}

template<typename Type>
inline bool ezVec4Template<Type>::IsZero() const
{
  EZ_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

template<typename Type>
inline bool ezVec4Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon) &&
          ezMath::IsZero (z, fEpsilon) &&
          ezMath::IsZero (w, fEpsilon));
}

template<typename Type>
inline bool ezVec4Template<Type>::IsNaN() const
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

template<typename Type>
inline bool ezVec4Template<Type>::IsValid() const
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

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec4Template<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(-x, -y, -z, -w);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator+= (const ezVec4Template<Type>& cc)
{
  x += cc.x;
  y += cc.y;
  z += cc.z;
  w += cc.w;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator-= (const ezVec4Template<Type>& cc)
{
  x -= cc.x;
  y -= cc.y;
  z -= cc.z;
  w -= cc.w;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator*= (Type f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator/= (Type f)
{
  const Type f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
  z *= f_inv;
  w *= f_inv;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::Dot(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

template<typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMin(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y), ezMath::Min(z, rhs.z), ezMath::Min(w, rhs.w));
}

template<typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMax(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y), ezMath::Max(z, rhs.z), ezMath::Max(w, rhs.w));
}

template<typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMult(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

template<typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompDiv(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator+ (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec4Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator- (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec4Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator* (Type f, const ezVec4Template<Type>& v)
{
  EZ_NAN_ASSERT(&v);

  return ezVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator* (const ezVec4Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  return ezVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator/ (const ezVec4Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  // multiplication is much faster than division
  const Type f_inv = ezMath::Invert(f);
  return ezVec4Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv, v.w * f_inv);
}

template<typename Type>
inline bool ezVec4Template<Type>::IsIdentical(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

template<typename Type>
inline bool ezVec4Template<Type>::IsEqual(const ezVec4Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsEqual(y, rhs.y, fEpsilon) && 
          ezMath::IsEqual(z, rhs.z, fEpsilon) &&
          ezMath::IsEqual(w, rhs.w, fEpsilon));
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator< (const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

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


