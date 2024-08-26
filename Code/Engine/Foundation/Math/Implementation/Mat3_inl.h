#pragma once

#include <Foundation/Memory/MemoryUtils.h>

template <typename Type>
ezMat3Template<Type>::ezMat3Template()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::NaN<Type>();
  for (ezUInt32 i = 0; i < 9; ++i)
    m_fElementsCM[i] = TypeNaN;
#endif
}

template <typename Type>
void ezMat3Template<Type>::GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const
{
  EZ_NAN_ASSERT(this);

  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(out_pData, m_fElementsCM, 9);
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      out_pData[i * 3 + 0] = Element(0, i);
      out_pData[i * 3 + 1] = Element(1, i);
      out_pData[i * 3 + 2] = Element(2, i);
    }
  }
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeZero()
{
  ezMat3Template<Type> res;

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(res.m_fElementsCM); ++i)
    res.m_fElementsCM[i] = 0.0f;

  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeIdentity()
{
  ezMat3Template<Type> res;
  res.m_fElementsCM[0] = 1.0f;
  res.m_fElementsCM[1] = 0.0f;
  res.m_fElementsCM[2] = 0.0f;
  res.m_fElementsCM[3] = 0.0f;
  res.m_fElementsCM[4] = 1.0f;
  res.m_fElementsCM[5] = 0.0f;
  res.m_fElementsCM[6] = 0.0f;
  res.m_fElementsCM[7] = 0.0f;
  res.m_fElementsCM[8] = 1.0f;
  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeFromRowMajorArray(const Type* const pData)
{
  ezMat3Template<Type> res;
  for (int i = 0; i < 3; ++i)
  {
    res.Element(0, i) = pData[i * 3 + 0];
    res.Element(1, i) = pData[i * 3 + 1];
    res.Element(2, i) = pData[i * 3 + 2];
  }
  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeFromColumnMajorArray(const Type* const pData)
{
  ezMat3Template<Type> res;
  ezMemoryUtils::Copy(res.m_fElementsCM, pData, 9);
  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeFromValues(Type c1r1, Type c2r1, Type c3r1, Type c1r2, Type c2r2, Type c3r2, Type c1r3, Type c2r3, Type c3r3)
{
  ezMat3Template<Type> res;
  res.Element(0, 0) = c1r1;
  res.Element(1, 0) = c2r1;
  res.Element(2, 0) = c3r1;
  res.Element(0, 1) = c1r2;
  res.Element(1, 1) = c2r2;
  res.Element(2, 1) = c3r2;
  res.Element(0, 2) = c1r3;
  res.Element(1, 2) = c2r3;
  res.Element(2, 2) = c3r3;
  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeScaling(const ezVec3Template<Type>& vScale)
{
  ezMat3Template<Type> res;
  res.Element(0, 0) = vScale.x;
  res.Element(1, 0) = 0;
  res.Element(2, 0) = 0;
  res.Element(0, 1) = 0;
  res.Element(1, 1) = vScale.y;
  res.Element(2, 1) = 0;
  res.Element(0, 2) = 0;
  res.Element(1, 2) = 0;
  res.Element(2, 2) = vScale.z;
  return res;
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeRotationX(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);

  return ezMat3Template<Type>::MakeFromValues(1.0f, 0.0f, 0.0f, 0.0f, fCos, -fSin, 0.0f, fSin, fCos);
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeRotationY(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);

  return ezMat3Template<Type>::MakeFromValues(fCos, 0.0f, fSin, 0.0f, 1.0f, 0.0f, -fSin, 0.0f, fCos);
}

template <typename Type>
ezMat3Template<Type> ezMat3Template<Type>::MakeRotationZ(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);

  return ezMat3Template<Type>::MakeFromValues(fCos, -fSin, 0.0f, fSin, fCos, 0.0f, 0.0f, 0.0f, 1.0f);
}

template <typename Type>
void ezMat3Template<Type>::SetZero()
{
  *this = MakeZero();
}

template <typename Type>
void ezMat3Template<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
void ezMat3Template<Type>::Transpose()
{
  ezMath::Swap(Element(0, 1), Element(1, 0));
  ezMath::Swap(Element(0, 2), Element(2, 0));
  ezMath::Swap(Element(1, 2), Element(2, 1));
}

template <typename Type>
const ezMat3Template<Type> ezMat3Template<Type>::GetTranspose() const
{
  return ezMat3Template::MakeFromRowMajorArray(m_fElementsCM);
}

template <typename Type>
const ezMat3Template<Type> ezMat3Template<Type>::GetInverse(Type fEpsilon) const
{
  ezMat3Template<Type> Inverse = *this;
  ezResult res = Inverse.Invert(fEpsilon);
  EZ_ASSERT_DEBUG(res.Succeeded(), "Could not invert the given Mat3.");
  EZ_IGNORE_UNUSED(res);
  return Inverse;
}

template <typename Type>
ezVec3Template<Type> ezMat3Template<Type>::GetRow(ezUInt32 uiRow) const
{
  EZ_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  ezVec3Template<Type> r;
  r.x = Element(0, uiRow);
  r.y = Element(1, uiRow);
  r.z = Element(2, uiRow);

  EZ_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void ezMat3Template<Type>::SetRow(ezUInt32 uiRow, const ezVec3Template<Type>& vRow)
{
  EZ_ASSERT_DEBUG(uiRow <= 2, "Invalid Row Index {0}", uiRow);

  Element(0, uiRow) = vRow.x;
  Element(1, uiRow) = vRow.y;
  Element(2, uiRow) = vRow.z;
}

template <typename Type>
ezVec3Template<Type> ezMat3Template<Type>::GetColumn(ezUInt32 uiColumn) const
{
  EZ_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  ezVec3Template<Type> r;
  r.x = Element(uiColumn, 0);
  r.y = Element(uiColumn, 1);
  r.z = Element(uiColumn, 2);

  EZ_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
void ezMat3Template<Type>::SetColumn(ezUInt32 uiColumn, const ezVec3Template<Type>& vColumn)
{
  EZ_ASSERT_DEBUG(uiColumn <= 2, "Invalid Column Index {0}", uiColumn);

  Element(uiColumn, 0) = vColumn.x;
  Element(uiColumn, 1) = vColumn.y;
  Element(uiColumn, 2) = vColumn.z;
}

template <typename Type>
ezVec3Template<Type> ezMat3Template<Type>::GetDiagonal() const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(Element(0, 0), Element(1, 1), Element(2, 2));
}

template <typename Type>
void ezMat3Template<Type>::SetDiagonal(const ezVec3Template<Type>& vDiag)
{
  Element(0, 0) = vDiag.x;
  Element(1, 1) = vDiag.y;
  Element(2, 2) = vDiag.z;
}

template <typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezMat3Template<Type>::TransformDirection(const ezVec3Template<Type>& v) const
{
  ezVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z;

  EZ_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
EZ_FORCE_INLINE void ezMat3Template<Type>::operator*=(Type f)
{
  for (ezInt32 i = 0; i < 9; ++i)
    m_fElementsCM[i] *= f;

  EZ_NAN_ASSERT(this);
}

template <typename Type>
EZ_FORCE_INLINE void ezMat3Template<Type>::operator/=(Type f)
{
  const Type fInv = ezMath::Invert(f);

  operator*=(fInv);
}

template <typename Type>
const ezMat3Template<Type> operator*(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2)
{
  ezMat3Template<Type> r;
  for (ezInt32 i = 0; i < 3; ++i)
  {
    r.Element(0, i) = m1.Element(0, i) * m2.Element(0, 0) + m1.Element(1, i) * m2.Element(0, 1) + m1.Element(2, i) * m2.Element(0, 2);
    r.Element(1, i) = m1.Element(0, i) * m2.Element(1, 0) + m1.Element(1, i) * m2.Element(1, 1) + m1.Element(2, i) * m2.Element(1, 2);
    r.Element(2, i) = m1.Element(0, i) * m2.Element(2, 0) + m1.Element(1, i) * m2.Element(2, 1) + m1.Element(2, i) * m2.Element(2, 2);
  }

  EZ_NAN_ASSERT(&r);
  return r;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezVec3Template<Type> operator*(const ezMat3Template<Type>& m, const ezVec3Template<Type>& v)
{
  return m.TransformDirection(v);
}



// *** free functions ***

template <typename Type>
EZ_ALWAYS_INLINE const ezMat3Template<Type> operator*(Type f, const ezMat3Template<Type>& m1)
{
  return operator*(m1, f);
}

template <typename Type>
const ezMat3Template<Type> operator*(const ezMat3Template<Type>& m1, Type f)
{
  ezMat3Template<Type> r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  EZ_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezMat3Template<Type> operator/(const ezMat3Template<Type>& m1, Type f)
{
  return operator*(m1, ezMath::Invert(f));
}

template <typename Type>
const ezMat3Template<Type> operator+(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2)
{
  ezMat3Template<Type> r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  EZ_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
const ezMat3Template<Type> operator-(const ezMat3Template<Type>& m1, const ezMat3Template<Type>& m2)
{
  ezMat3Template<Type> r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  EZ_NAN_ASSERT(&r);

  return r;
}

template <typename Type>
bool ezMat3Template<Type>::IsIdentical(const ezMat3Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

template <typename Type>
bool ezMat3Template<Type>::IsEqual(const ezMat3Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  EZ_ASSERT_DEBUG(fEpsilon >= 0.0f, "Epsilon may not be negative.");

  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (!ezMath::IsEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator==(const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator!=(const ezMat3Template<Type>& lhs, const ezMat3Template<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template <typename Type>
bool ezMat3Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (!ezMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template <typename Type>
bool ezMat3Template<Type>::IsIdentity(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  if (!ezMath::IsEqual(Element(0, 0), (Type)1, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(0, 1), (Type)0, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(0, 2), (Type)0, fEpsilon))
    return false;

  if (!ezMath::IsEqual(Element(1, 0), (Type)0, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(1, 1), (Type)1, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(1, 2), (Type)0, fEpsilon))
    return false;

  if (!ezMath::IsEqual(Element(2, 0), (Type)0, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(2, 1), (Type)0, fEpsilon))
    return false;
  if (!ezMath::IsEqual(Element(2, 2), (Type)1, fEpsilon))
    return false;

  return true;
}

template <typename Type>
bool ezMat3Template<Type>::IsValid() const
{
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (!ezMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}

template <typename Type>
bool ezMat3Template<Type>::IsNaN() const
{
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (ezMath::IsNaN(m_fElementsCM[i]))
      return true;
  }

  return false;
}

template <typename Type>
const ezVec3Template<Type> ezMat3Template<Type>::GetScalingFactors() const
{
  ezVec3Template<Type> v;

  v.x = ezVec3Template<Type>(Element(0, 0), Element(0, 1), Element(0, 2)).GetLength();
  v.y = ezVec3Template<Type>(Element(1, 0), Element(1, 1), Element(1, 2)).GetLength();
  v.z = ezVec3Template<Type>(Element(2, 0), Element(2, 1), Element(2, 2)).GetLength();

  EZ_NAN_ASSERT(&v);
  return v;
}

template <typename Type>
ezResult ezMat3Template<Type>::SetScalingFactors(const ezVec3Template<Type>& vXYZ, Type fEpsilon /* = ezMath::DefaultEpsilon<Type>() */)
{
  ezVec3Template<Type> tx(Element(0, 0), Element(0, 1), Element(0, 2));
  ezVec3Template<Type> ty(Element(1, 0), Element(1, 1), Element(1, 2));
  ezVec3Template<Type> tz(Element(2, 0), Element(2, 1), Element(2, 2));

  if (tx.SetLength(vXYZ.x, fEpsilon) == EZ_FAILURE)
    return EZ_FAILURE;
  if (ty.SetLength(vXYZ.y, fEpsilon) == EZ_FAILURE)
    return EZ_FAILURE;
  if (tz.SetLength(vXYZ.z, fEpsilon) == EZ_FAILURE)
    return EZ_FAILURE;


  Element(0, 0) = tx.x;
  Element(0, 1) = tx.y;
  Element(0, 2) = tx.z;
  Element(1, 0) = ty.x;
  Element(1, 1) = ty.y;
  Element(1, 2) = ty.z;
  Element(2, 0) = tz.x;
  Element(2, 1) = tz.y;
  Element(2, 2) = tz.z;

  return EZ_SUCCESS;
}

template <typename Type>
Type ezMat3Template<Type>::GetDeterminant() const
{
  // Using rule of Sarrus
  Type fDeterminant = 0;
  for (int i = 0; i < 3; i++)
  {
    fDeterminant += Element(i, 0) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 2);
    fDeterminant -= Element(i, 2) * Element((i + 1) % 3, 1) * Element((i + 2) % 3, 0);
  }
  return fDeterminant;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>
