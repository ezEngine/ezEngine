#pragma once

template<typename Type>
EZ_FORCE_INLINE ezVec2Template<Type>::ezVec2Template()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  x = TypeNaN;
  y = TypeNaN;
#endif
}

template<typename Type>
EZ_FORCE_INLINE ezVec2Template<Type>::ezVec2Template(Type X, Type Y) : x (X), y (Y)
{
}

template<typename Type>
EZ_FORCE_INLINE ezVec2Template<Type>::ezVec2Template(Type V) : x (V), y (V)
{
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::Set(Type xy)
{
  x = xy;
  y = xy; 
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::Set(Type X, Type Y)
{
  x = X;
  y = Y;
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::SetZero()
{
  x = y = 0;
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec2Template<Type>::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec2Template<Type>::GetLengthSquared() const
{
  return (x * x + y * y);
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec2Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength ();

  const Type fLengthInv = ezMath::Invert(fLen);
  return ezVec2Template<Type>(x * fLengthInv, y * fLengthInv);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::Normalize()
{
  *this /= GetLength ();
}

template<typename Type>
inline ezResult ezVec2Template<Type>::NormalizeIfNotZero(const ezVec2Template<Type>& vFallback, Type fEpsilon)
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
inline bool ezVec2Template<Type>::IsNormalized(Type fEpsilon /* = ezMath::BasicType<Type>::HugeEpsilon() */) const
{
  const Type t = GetLengthSquared ();
  return ezMath::IsEqual(t, (Type) (1), fEpsilon);
}

template<typename Type>
inline bool ezVec2Template<Type>::IsZero() const
{
  return (x == 0 && y == 0);
}

template<typename Type>
inline bool ezVec2Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon));
}

template<typename Type>
inline bool ezVec2Template<Type>::IsNaN() const
{
  if (ezMath::IsNaN(x))
    return true;
  if (ezMath::IsNaN(y))
    return true;

  return false;
}

template<typename Type>
inline bool ezVec2Template<Type>::IsValid() const
{
  if (!ezMath::IsFinite(x))
    return false;
  if (!ezMath::IsFinite (y))
    return false;

  return true;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return ezVec2Template<Type>(-x, -y);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::operator+= (const ezVec2Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::operator-= (const ezVec2Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::operator*= (Type f)
{
  x *= f;
  y *= f;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec2Template<Type>::operator/= (Type f)
{
  const Type f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
inline void ezVec2Template<Type>::MakeOrthogonalTo (const ezVec2Template<Type>& vNormal)
{
  EZ_ASSERT_DEBUG(vNormal.IsNormalized (), "The normal must be normalized.");

  const Type fDot = this->Dot (vNormal);
  *this -= fDot * vNormal;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::GetOrthogonalVector() const
{
  EZ_NAN_ASSERT(this);
  EZ_ASSERT_DEBUG(!IsZero(ezMath::BasicType<Type>::SmallEpsilon()), "The vector must not be zero to be able to compute an orthogonal vector.");

  return ezVec2Template<Type>(-y, x);
}

template<typename Type>
inline const ezVec2Template<Type> ezVec2Template<Type>::GetReflectedVector(const ezVec2Template<Type>& vNormal) const 
{
  EZ_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - (2 * this->Dot(vNormal) * vNormal));
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec2Template<Type>::Dot(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y));
}

template<typename Type>
inline ezAngle ezVec2Template<Type>::GetAngleBetween(const ezVec2Template<Type>& rhs) const
{
  EZ_ASSERT_DEBUG (this->IsNormalized(), "This vector must be normalized.");
  EZ_ASSERT_DEBUG (rhs.IsNormalized(), "The other vector must be normalized.");

  return ezMath::ACos(ezMath::Clamp<Type>(this->Dot(rhs), (Type) -1, (Type) 1));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::CompMin(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec2Template<Type>(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::CompMax(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec2Template<Type>(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::CompMult(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec2Template<Type>(x * rhs.x, y * rhs.y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> ezVec2Template<Type>::CompDiv(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec2Template<Type>(x / rhs.x, y / rhs.y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> operator+ (const ezVec2Template<Type>& v1, const ezVec2Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec2Template<Type>(v1.x + v2.x, v1.y + v2.y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> operator- (const ezVec2Template<Type>& v1, const ezVec2Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec2Template<Type>(v1.x - v2.x, v1.y - v2.y);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> operator* (Type f, const ezVec2Template<Type>& v)
{
  EZ_NAN_ASSERT(&v);

  return ezVec2Template<Type>(v.x * f, v.y * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> operator* (const ezVec2Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  return ezVec2Template<Type>(v.x * f, v.y * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec2Template<Type> operator/ (const ezVec2Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  // multiplication is much faster than division
  const Type f_inv = ezMath::Invert(f);
  return ezVec2Template<Type>(v.x * f_inv, v.y * f_inv);
}

template<typename Type>
inline bool ezVec2Template<Type>::IsIdentical(const ezVec2Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y));
}

template<typename Type>
inline bool ezVec2Template<Type>::IsEqual(const ezVec2Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsEqual(y, rhs.y, fEpsilon));
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezVec2Template<Type>& v1, const ezVec2Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezVec2Template<Type>& v1, const ezVec2Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator< (const ezVec2Template<Type>& v1, const ezVec2Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  if (v1.x < v2.x)
    return true;
  if (v1.x > v2.x)
    return false;

  return (v1.y < v2.y);
}


