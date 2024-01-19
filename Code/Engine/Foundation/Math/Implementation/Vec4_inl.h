#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>

// *** Vec2 and Vec3 Code ***
// Cannot put this into the Vec3_inl.h file, that would result in circular dependencies

template <typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec2Template<Type>::GetAsVec3(Type z) const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(x, y, z);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec2Template<Type>::GetAsVec4(Type z, Type w) const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, w);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec3Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 3rd and 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // EZ_NAN_ASSERT(this);

  return ezVec2Template<Type>(x, y);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsVec4(Type w) const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, w);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsPositionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, 1);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec3Template<Type>::GetAsDirectionVec4() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(x, y, z, 0);
}

// *****************

template <typename Type>
EZ_ALWAYS_INLINE ezVec4Template<Type>::ezVec4Template()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::NaN<Type>();
  x = TypeNaN;
  y = TypeNaN;
  z = 0;
  w = 0;
#endif
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec4Template<Type>::ezVec4Template(Type x, Type y, Type z, Type w)
  : x(x)
  , y(y)
  , z(z)
  , w(w)
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec4Template<Type>::ezVec4Template(ezVec3Template<Type> vXyz, Type w)
  : x(vXyz.x)
  , y(vXyz.y)
  , z(vXyz.z)
  , w(w)
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec4Template<Type>::ezVec4Template(Type v)
  : x(v)
  , y(v)
  , z(v)
  , w(v)
{
}

template <typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec4Template<Type>::GetAsVec2() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // EZ_NAN_ASSERT(this);

  return ezVec2Template<Type>(x, y);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec4Template<Type>::GetAsVec3() const
{
  // don't assert here, as the 4th component may be NaN when this is fine, e.g. during interop with the SIMD classes
  // EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(x, y, z);
}

template <typename Type>
EZ_ALWAYS_INLINE void ezVec4Template<Type>::Set(Type xyzw)
{
  x = xyzw;
  y = xyzw;
  z = xyzw;
  w = xyzw;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezVec4Template<Type>::Set(Type inX, Type inY, Type inZ, Type inW)
{
  x = inX;
  y = inY;
  z = inZ;
  w = inW;
}

template <typename Type>
inline void ezVec4Template<Type>::SetZero()
{
  x = y = z = w = 0;
}

template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE EZ_ALWAYS_INLINE Type ezVec4Template<Type>::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

template <typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::GetLengthSquared() const
{
  EZ_NAN_ASSERT(this);

  return (x * x + y * y + z * z + w * w);
}

template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE EZ_FORCE_INLINE Type ezVec4Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE EZ_FORCE_INLINE const ezVec4Template<Type> ezVec4Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength();

  const Type fLengthInv = ezMath::Invert(fLen);
  return ezVec4Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv, w * fLengthInv);
}

template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE EZ_ALWAYS_INLINE void ezVec4Template<Type>::Normalize()
{
  *this /= GetLength();
}

template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE inline ezResult ezVec4Template<Type>::NormalizeIfNotZero(const ezVec4Template<Type>& vFallback, Type fEpsilon)
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
template <typename Type>
EZ_IMPLEMENT_IF_FLOAT_TYPE inline bool ezVec4Template<Type>::IsNormalized(Type fEpsilon /* = ezMath::HugeEpsilon<Type>() */) const
{
  const Type t = GetLengthSquared();
  return ezMath::IsEqual(t, (Type)1, fEpsilon);
}

template <typename Type>
inline bool ezVec4Template<Type>::IsZero() const
{
  EZ_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f) && (w == 0.0f));
}

template <typename Type>
inline bool ezVec4Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  return (ezMath::IsZero(x, fEpsilon) && ezMath::IsZero(y, fEpsilon) && ezMath::IsZero(z, fEpsilon) && ezMath::IsZero(w, fEpsilon));
}

template <typename Type>
inline bool ezVec4Template<Type>::IsNaN() const
{
  if (ezMath::IsNaN(x))
    return true;
  if (ezMath::IsNaN(y))
    return true;
  if (ezMath::IsNaN(z))
    return true;
  if (ezMath::IsNaN(w))
    return true;

  return false;
}

template <typename Type>
inline bool ezVec4Template<Type>::IsValid() const
{
  if (!ezMath::IsFinite(x))
    return false;
  if (!ezMath::IsFinite(y))
    return false;
  if (!ezMath::IsFinite(z))
    return false;
  if (!ezMath::IsFinite(w))
    return false;

  return true;
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> ezVec4Template<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(-x, -y, -z, -w);
}

template <typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator+=(const ezVec4Template<Type>& vCc)
{
  x += vCc.x;
  y += vCc.y;
  z += vCc.z;
  w += vCc.w;

  EZ_NAN_ASSERT(this);
}

template <typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator-=(const ezVec4Template<Type>& vCc)
{
  x -= vCc.x;
  y -= vCc.y;
  z -= vCc.z;
  w -= vCc.w;

  EZ_NAN_ASSERT(this);
}

template <typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator*=(Type f)
{
  x *= f;
  y *= f;
  z *= f;
  w *= f;

  EZ_NAN_ASSERT(this);
}

template <typename Type>
EZ_FORCE_INLINE void ezVec4Template<Type>::operator/=(Type f)
{
  if constexpr (std::is_floating_point_v<Type>)
  {
    const Type f_inv = ezMath::Invert(f);
    x *= f_inv;
    y *= f_inv;
    z *= f_inv;
    w *= f_inv;
  }
  else
  {
    x /= f;
    y /= f;
    z /= f;
    w /= f;
  }

  EZ_NAN_ASSERT(this);
}

template <typename Type>
EZ_FORCE_INLINE Type ezVec4Template<Type>::Dot(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z) + (w * rhs.w));
}

template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMin(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y), ezMath::Min(z, rhs.z), ezMath::Min(w, rhs.w));
}

template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMax(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y), ezMath::Max(z, rhs.z), ezMath::Max(w, rhs.w));
}

template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompClamp(const ezVec4Template& vLow, const ezVec4Template& vHigh) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&vLow);
  EZ_NAN_ASSERT(&vHigh);

  return ezVec4Template<Type>(ezMath::Clamp(x, vLow.x, vHigh.x), ezMath::Clamp(y, vLow.y, vHigh.y), ezMath::Clamp(z, vLow.z, vHigh.z), ezMath::Clamp(w, vLow.w, vHigh.w));
}

template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompMul(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

EZ_MSVC_ANALYSIS_WARNING_PUSH
EZ_MSVC_ANALYSIS_WARNING_DISABLE(4723)
template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::CompDiv(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec4Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w);
}
EZ_MSVC_ANALYSIS_WARNING_POP

template <typename Type>
inline const ezVec4Template<Type> ezVec4Template<Type>::Abs() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(ezMath::Abs(x), ezMath::Abs(y), ezMath::Abs(z), ezMath::Abs(w));
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator+(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec4Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator-(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec4Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator*(Type f, const ezVec4Template<Type>& v)
{
  EZ_NAN_ASSERT(&v);

  return ezVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator*(const ezVec4Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  return ezVec4Template<Type>(v.x * f, v.y * f, v.z * f, v.w * f);
}

template <typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator/(const ezVec4Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  if constexpr (std::is_floating_point_v<Type>)
  {
    // multiplication is much faster than division
    const Type f_inv = ezMath::Invert(f);
    return ezVec4Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv, v.w * f_inv);
  }
  else
  {
    return ezVec4Template<Type>(v.x / f, v.y / f, v.z / f, v.w / f);
  }
}

template <typename Type>
inline bool ezVec4Template<Type>::IsIdentical(const ezVec4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w));
}

template <typename Type>
inline bool ezVec4Template<Type>::IsEqual(const ezVec4Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(x, rhs.x, fEpsilon) && ezMath::IsEqual(y, rhs.y, fEpsilon) && ezMath::IsEqual(z, rhs.z, fEpsilon) && ezMath::IsEqual(w, rhs.w, fEpsilon));
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator==(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator!=(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template <typename Type>
EZ_FORCE_INLINE bool operator<(const ezVec4Template<Type>& v1, const ezVec4Template<Type>& v2)
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
