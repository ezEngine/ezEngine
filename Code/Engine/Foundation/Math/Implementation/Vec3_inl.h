#pragma once

template<typename Type>
EZ_FORCE_INLINE ezVec3Template<Type>::ezVec3Template()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  x = TypeNaN;
  y = TypeNaN;
  z = TypeNaN;
#endif
}

template<typename Type>
EZ_FORCE_INLINE ezVec3Template<Type>::ezVec3Template(Type X, Type Y, Type Z) : x (X), y (Y), z (Z)
{
}

template<typename Type>
EZ_FORCE_INLINE ezVec3Template<Type>::ezVec3Template(Type V) : x (V), y (V), z (V)
{
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::Set(Type xyz)
{
  x = xyz;
  y = xyz;
  z = xyz;
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::Set(Type X, Type Y, Type Z)
{
  x = X;
  y = Y;
  z = Z;
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::SetZero()
{
  x = y = z = 0.0f;
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec3Template<Type>::GetLength() const
{
  return (ezMath::Sqrt(GetLengthSquared()));
}

template<typename Type>
ezResult ezVec3Template<Type>::SetLength(Type fNewLength, Type fEpsilon /* = ezMath::BasicType<Type>::DefaultEpsilon() */)
{
  if (NormalizeIfNotZero(ezVec3Template<Type>::ZeroVector(), fEpsilon) == EZ_FAILURE)
    return EZ_FAILURE;

  *this *= fNewLength;
  return EZ_SUCCESS;
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec3Template<Type>::GetLengthSquared() const
{
  EZ_NAN_ASSERT(this);

  return (x * x + y * y + z * z);
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec3Template<Type>::GetLengthAndNormalize()
{
  const Type fLength = GetLength();
  *this /= fLength;
  return fLength;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::GetNormalized() const
{
  const Type fLen = GetLength ();

  const Type fLengthInv = ezMath::Invert(fLen);
  return ezVec3Template<Type>(x * fLengthInv, y * fLengthInv, z * fLengthInv);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::Normalize()
{
  *this /= GetLength ();
}

template<typename Type>
ezResult ezVec3Template<Type>::NormalizeIfNotZero(const ezVec3Template<Type>& vFallback, Type fEpsilon)
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
EZ_FORCE_INLINE bool ezVec3Template<Type>::IsNormalized(Type fEpsilon /* = ezMath::BasicType<Type>::HugeEpsilon() */) const
{
  const Type t = GetLengthSquared ();
  return ezMath::IsEqual(t, (Type) 1, fEpsilon);
}

template<typename Type>
EZ_FORCE_INLINE bool ezVec3Template<Type>::IsZero() const
{
  EZ_NAN_ASSERT(this);

  return ((x == 0.0f) && (y == 0.0f) && (z == 0.0f));
}

template<typename Type>
bool ezVec3Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  return (ezMath::IsZero (x, fEpsilon) &&
          ezMath::IsZero (y, fEpsilon) &&
          ezMath::IsZero (z, fEpsilon));
}

template<typename Type>
bool ezVec3Template<Type>::IsNaN() const
{
  if (ezMath::IsNaN(x))
    return true;
  if (ezMath::IsNaN(y))
    return true;
  if (ezMath::IsNaN(z))
    return true;

  return false;
}

template<typename Type>
bool ezVec3Template<Type>::IsValid() const
{
  if (!ezMath::IsFinite(x))
    return false;
  if (!ezMath::IsFinite(y))
    return false;
  if (!ezMath::IsFinite(z))
    return false;

  return true;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::operator-() const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(-x, -y, -z);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::operator+= (const ezVec3Template<Type>& rhs)
{
  x += rhs.x;
  y += rhs.y;
  z += rhs.z;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::operator-= (const ezVec3Template<Type>& rhs)
{
  x -= rhs.x;
  y -= rhs.y;
  z -= rhs.z;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::operator*= (Type f)
{
  x *= f;
  y *= f;
  z *= f;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
EZ_FORCE_INLINE void ezVec3Template<Type>::operator/= (Type f)
{
  const Type f_inv = ezMath::Invert(f);

  x *= f_inv;
  y *= f_inv;
  z *= f_inv;

  // if this assert fires, you might have tried to normalize a zero-length vector
  EZ_NAN_ASSERT(this);
}

template<typename Type>
ezResult ezVec3Template<Type>::CalculateNormal(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3)
{
  *this = (v3 - v2).Cross(v1 - v2);
  return NormalizeIfNotZero();
}

template<typename Type>
void ezVec3Template<Type>::MakeOrthogonalTo(const ezVec3Template<Type>& vNormal)
{
  EZ_ASSERT_DEBUG(vNormal.IsNormalized(), "The vector to make this vector orthogonal to, must be normalized. It's length is %.3f", vNormal.GetLength());

  ezVec3Template<Type> vOrtho = vNormal.Cross(*this);
  *this = vOrtho.Cross(vNormal);
}

template<typename Type>
const ezVec3Template<Type> ezVec3Template<Type>::GetOrthogonalVector() const
{
  EZ_ASSERT_DEBUG(!IsZero(ezMath::BasicType<Type>::SmallEpsilon()), "The vector must not be zero to be able to compute an orthogonal vector.");

  Type fDot = ezMath::Abs(this->Dot (ezVec3Template<Type> (0, 1, 0)));
  if (fDot < 0.999f)
    return this->Cross(ezVec3Template<Type> (0, 1, 0));
   
  return this->Cross(ezVec3Template<Type> (1, 0, 0));
}

template<typename Type>
const ezVec3Template<Type> ezVec3Template<Type>::GetReflectedVector(const ezVec3Template<Type>& vNormal) const 
{
  EZ_ASSERT_DEBUG(vNormal.IsNormalized(), "vNormal must be normalized.");

  return ((*this) - ((Type) 2 * this->Dot (vNormal) * vNormal));
}

template<typename Type>
EZ_FORCE_INLINE Type ezVec3Template<Type>::Dot(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x * rhs.x) + (y * rhs.y) + (z * rhs.z));
}

template<typename Type>
const ezVec3Template<Type> ezVec3Template<Type>::Cross(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec3Template<Type>(y * rhs.z - z * rhs.y,
                              z * rhs.x - x * rhs.z,
                              x * rhs.y - y * rhs.x);
}

template<typename Type>
ezAngle ezVec3Template<Type>::GetAngleBetween(const ezVec3Template<Type>& rhs) const
{
  EZ_ASSERT_DEBUG (this->IsNormalized(), "This vector must be normalized.");
  EZ_ASSERT_DEBUG (rhs.IsNormalized(), "The other vector must be normalized.");

  return ezMath::ACos(ezMath::Clamp(this->Dot (rhs), (Type) -1, (Type) 1));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::CompMin(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec3Template<Type>(ezMath::Min(x, rhs.x), ezMath::Min(y, rhs.y), ezMath::Min(z, rhs.z));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::CompMax(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec3Template<Type>(ezMath::Max(x, rhs.x), ezMath::Max(y, rhs.y), ezMath::Max(z, rhs.z));
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::CompMult(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec3Template<Type>(x * rhs.x, y * rhs.y, z * rhs.z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezVec3Template<Type>::CompDiv(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ezVec3Template<Type>(x / rhs.x, y / rhs.y, z / rhs.z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator+ (const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec3Template<Type>(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator- (const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2)
{
  EZ_NAN_ASSERT(&v1);
  EZ_NAN_ASSERT(&v2);

  return ezVec3Template<Type>(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator* (Type f, const ezVec3Template<Type>& v)
{
  EZ_NAN_ASSERT(&v);

  return ezVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator* (const ezVec3Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  return ezVec3Template<Type>(v.x * f, v.y * f, v.z * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator/ (const ezVec3Template<Type>& v, Type f)
{
  EZ_NAN_ASSERT(&v);

  // multiplication is much faster than division
  const Type f_inv = ezMath::Invert(f);
  return ezVec3Template<Type>(v.x * f_inv, v.y * f_inv, v.z * f_inv);
}

template<typename Type>
bool ezVec3Template<Type>::IsIdentical(const ezVec3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return ((x == rhs.x) && (y == rhs.y) && (z == rhs.z));
}

template<typename Type>
bool ezVec3Template<Type>::IsEqual(const ezVec3Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(x, rhs.x, fEpsilon) && 
          ezMath::IsEqual(y, rhs.y, fEpsilon) && 
          ezMath::IsEqual(z, rhs.z, fEpsilon));
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2)
{
  return v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2)
{
  return !v1.IsIdentical(v2);
}

template<typename Type>
EZ_FORCE_INLINE bool operator< (const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2)
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

  return (v1.z < v2.z);
}

template<typename Type>
const ezVec3Template<Type> ezVec3Template<Type>::GetRefractedVector (const ezVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const
{
  EZ_ASSERT_DEBUG (vNormal.IsNormalized(), "vNormal must be normalized.");

  const Type n = fRefIndex1 / fRefIndex2;
  const Type cosI = this->Dot (vNormal);
  const Type sinT2 = n * n * (1.0f - (cosI * cosI));

  //invalid refraction
  if (sinT2 > 1.0f)
    return (*this);

  return ((n * (*this)) - (n + ezMath::Sqrt (1.0f - sinT2)) * vNormal);
}

