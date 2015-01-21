#pragma once

#include <Foundation/Math/Mat3.h>

template<typename Type>
ezMat4Template<Type>::ezMat4Template()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  SetElements(TypeNaN, TypeNaN, TypeNaN, TypeNaN,
              TypeNaN, TypeNaN, TypeNaN, TypeNaN,
              TypeNaN, TypeNaN, TypeNaN, TypeNaN,
              TypeNaN, TypeNaN, TypeNaN, TypeNaN);
#endif
}

template<typename Type>
ezMat4Template<Type>::ezMat4Template(const Type* const pData, ezMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

template<typename Type>
ezMat4Template<Type>::ezMat4Template(Type c1r1, Type c2r1, Type c3r1, Type c4r1,
                                     Type c1r2, Type c2r2, Type c3r2, Type c4r2,
                                     Type c1r3, Type c2r3, Type c3r3, Type c4r3,
                                     Type c1r4, Type c2r4, Type c3r4, Type c4r4)
{
  SetElements(c1r1, c2r1, c3r1, c4r1,
              c1r2, c2r2, c3r2, c4r2,
              c1r3, c2r3, c3r3, c4r3,
              c1r4, c2r4, c3r4, c4r4);
}

template<typename Type>
ezMat4Template<Type>::ezMat4Template(const ezMat3Template<Type>& Rotation, const ezVec3Template<Type>& vTranslation)
{
  SetTransformationMatrix(Rotation, vTranslation);
}

template<typename Type>
const ezMat4Template<Type> ezMat4Template<Type>::IdentityMatrix()
{
  return ezMat4Template(1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
}

template<typename Type>
const ezMat4Template<Type> ezMat4Template<Type>::ZeroMatrix()
{
  return ezMat4Template(0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0);
}

template<typename Type>
void ezMat4Template<Type>::SetFromArray(const Type* EZ_RESTRICT const pData, ezMatrixLayout::Enum layout)
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(m_fElementsCM, pData, 16);
  }
  else
  {
    for (int i = 0; i < 4; ++i)
    {
      Element(0, i) = pData[i * 4 + 0];
      Element(1, i) = pData[i * 4 + 1];
      Element(2, i) = pData[i * 4 + 2];
      Element(3, i) = pData[i * 4 + 3];
    }
  }
}

template<typename Type>
void ezMat4Template<Type>::SetTransformationMatrix(const ezMat3Template<Type>& Rotation, const ezVec3Template<Type>& vTranslation)
{
  SetRotationalPart(Rotation);
  SetTranslationVector(vTranslation);
  SetRow(3, ezVec4Template<Type>(0, 0, 0, 1));
}

template<typename Type>
void ezMat4Template<Type>::GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const
{
  EZ_NAN_ASSERT(this);

  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(out_pData, m_fElementsCM, 16);
  }
  else
  {
    for (int i = 0; i < 4; ++i)
    {
      out_pData[i * 4 + 0] = Element(0, i);
      out_pData[i * 4 + 1] = Element(1, i);
      out_pData[i * 4 + 2] = Element(2, i);
      out_pData[i * 4 + 3] = Element(3, i);
    }
  }
}

template<typename Type>
void ezMat4Template<Type>::SetElements(Type c1r1, Type c2r1, Type c3r1, Type c4r1,
                                Type c1r2, Type c2r2, Type c3r2, Type c4r2,
                                Type c1r3, Type c2r3, Type c3r3, Type c4r3,
                                Type c1r4, Type c2r4, Type c3r4, Type c4r4)
{
  Element(0, 0) = c1r1;	Element(1, 0) = c2r1;	Element(2, 0) = c3r1;	Element(3, 0) = c4r1;
  Element(0, 1) = c1r2;	Element(1, 1) = c2r2;	Element(2, 1) = c3r2;	Element(3, 1) = c4r2;
  Element(0, 2) = c1r3;	Element(1, 2) = c2r3;	Element(2, 2) = c3r3;	Element(3, 2) = c4r3;
  Element(0, 3) = c1r4;	Element(1, 3) = c2r4;	Element(2, 3) = c3r4;	Element(3, 3) = c4r4;
}

template<typename Type>
void ezMat4Template<Type>::SetZero()
{
  SetElements(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0);
}

template<typename Type>
void ezMat4Template<Type>::SetIdentity()
{
  SetElements(1, 0, 0, 0,
              0, 1, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1);
}

template<typename Type>
void ezMat4Template<Type>::SetTranslationMatrix(const ezVec3Template<Type>& vTranslation)
{
  SetElements(1, 0, 0, vTranslation.x,
              0, 1, 0, vTranslation.y,
              0, 0, 1, vTranslation.z,
              0, 0, 0, 1);
}

template<typename Type>
void ezMat4Template<Type>::SetScalingMatrix(const ezVec3Template<Type>& s)
{
  SetElements(s.x,  0,   0,   0,
               0,  s.y,  0,   0,
               0,   0,  s.z,  0,
               0,   0,   0,   1);
}

template<typename Type>
void ezMat4Template<Type>::SetRotationMatrixX(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);

  SetElements(1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, fCos,-fSin, 0.0f,
            0.0f, fSin, fCos, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

template<typename Type>
void ezMat4Template<Type>::SetRotationMatrixY(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);


  SetElements(fCos, 0.0f, fSin, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
           -fSin, 0.0f, fCos, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

template<typename Type>
void ezMat4Template<Type>::SetRotationMatrixZ(ezAngle angle)
{
  const Type fSin = ezMath::Sin(angle);
  const Type fCos = ezMath::Cos(angle);

  SetElements(fCos,-fSin, 0.0f, 0.0f,
            fSin, fCos, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

template<typename Type>
void ezMat4Template<Type>::Transpose()
{
  ezMath::Swap(Element(0, 1), Element(1, 0));
  ezMath::Swap(Element(0, 2), Element(2, 0));
  ezMath::Swap(Element(0, 3), Element(3, 0));
  ezMath::Swap(Element(1, 2), Element(2, 1));
  ezMath::Swap(Element(1, 3), Element(3, 1));
  ezMath::Swap(Element(2, 3), Element(3, 2));
}

template<typename Type>
const ezMat4Template<Type> ezMat4Template<Type>::GetTranspose() const
{
  EZ_NAN_ASSERT(this);

  return ezMat4Template(m_fElementsCM, ezMatrixLayout::RowMajor);
}

template<typename Type>
const ezMat4Template<Type> ezMat4Template<Type>::GetInverse() const
{
  ezMat4Template<Type> Inverse = *this;
  Inverse.Invert();
  return Inverse;
}

template<typename Type>
ezVec4Template<Type> ezMat4Template<Type>::GetRow(ezUInt32 uiRow) const
{
  EZ_NAN_ASSERT(this);
  EZ_ASSERT_DEBUG(uiRow <= 3, "Invalid Row Index %d", uiRow);

  ezVec4Template<Type> r;
  r.x = Element(0, uiRow);
  r.y = Element(1, uiRow);
  r.z = Element(2, uiRow);
  r.w = Element(3, uiRow);

  return r;
}

template<typename Type>
void ezMat4Template<Type>::SetRow(ezUInt32 uiRow, const ezVec4Template<Type>& row)
{
  EZ_ASSERT_DEBUG(uiRow <= 3, "Invalid Row Index %d", uiRow);

  Element(0, uiRow) = row.x;
  Element(1, uiRow) = row.y;
  Element(2, uiRow) = row.z;
  Element(3, uiRow) = row.w;
}

template<typename Type>
ezVec4Template<Type> ezMat4Template<Type>::GetColumn(ezUInt32 uiColumn) const
{
  EZ_NAN_ASSERT(this);
  EZ_ASSERT_DEBUG(uiColumn <= 3, "Invalid Column Index %d", uiColumn);

  ezVec4Template<Type> r;
  r.x = Element(uiColumn, 0);
  r.y = Element(uiColumn, 1);
  r.z = Element(uiColumn, 2);
  r.w = Element(uiColumn, 3);

  return r;
}

template<typename Type>
void ezMat4Template<Type>::SetColumn(ezUInt32 uiColumn, const ezVec4Template<Type>& column)
{
  EZ_ASSERT_DEBUG(uiColumn <= 3, "Invalid Column Index %d", uiColumn);

  Element(uiColumn, 0) = column.x;
  Element(uiColumn, 1) = column.y;
  Element(uiColumn, 2) = column.z;
  Element(uiColumn, 3) = column.w;
}

template<typename Type>
ezVec4Template<Type> ezMat4Template<Type>::GetDiagonal() const
{
  EZ_NAN_ASSERT(this);

  return ezVec4Template<Type>(Element(0, 0), Element(1, 1), Element(2, 2), Element(3, 3));
}

template<typename Type>
void ezMat4Template<Type>::SetDiagonal(const ezVec4Template<Type>& diag)
{
  Element(0, 0) = diag.x;
  Element(1, 1) = diag.y;
  Element(2, 2) = diag.z;
  Element(3, 3) = diag.w;
}

template<typename Type>
const ezVec3Template<Type> ezMat4Template<Type>::TransformPosition(const ezVec3Template<Type>& v) const
{
  ezVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z + Element(3, 0);
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z + Element(3, 1);
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z + Element(3, 2);

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
void ezMat4Template<Type>::TransformPosition(ezVec3Template<Type>* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec3Template) */) const
{
  EZ_ASSERT_DEBUG(inout_v != nullptr, "Array must not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "Data must not overlap.");

  ezVec3Template<Type>* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformPosition(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template<typename Type>
const ezVec3Template<Type> ezMat4Template<Type>::TransformDirection(const ezVec3Template<Type>& v) const
{
  ezVec3Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z;

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
void ezMat4Template<Type>::TransformDirection(ezVec3Template<Type>* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec3Template<Type>) */) const
{
  EZ_ASSERT_DEBUG(inout_v != nullptr, "Array must not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "Data must not overlap.");

  ezVec3Template<Type>* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformDirection(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template<typename Type>
const ezVec4Template<Type> ezMat4Template<Type>::Transform(const ezVec4Template<Type>& v) const
{
  ezVec4Template<Type> r;
  r.x = Element(0, 0) * v.x + Element(1, 0) * v.y + Element(2, 0) * v.z + Element(3, 0) * v.w;
  r.y = Element(0, 1) * v.x + Element(1, 1) * v.y + Element(2, 1) * v.z + Element(3, 1) * v.w;
  r.z = Element(0, 2) * v.x + Element(1, 2) * v.y + Element(2, 2) * v.z + Element(3, 2) * v.w;
  r.w = Element(0, 3) * v.x + Element(1, 3) * v.y + Element(2, 3) * v.z + Element(3, 3) * v.w;

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
void ezMat4Template<Type>::Transform(ezVec4Template<Type>* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec4Template) */) const
{
  EZ_ASSERT_DEBUG(inout_v != nullptr, "Array must not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec4Template<Type>), "Data must not overlap.");

  ezVec4Template<Type>* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = Transform(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezMat4Template<Type>::GetTranslationVector() const
{
  EZ_NAN_ASSERT(this);

  return ezVec3Template<Type>(Element(3, 0), Element(3, 1), Element(3, 2));
}

template<typename Type>
EZ_FORCE_INLINE void ezMat4Template<Type>::SetTranslationVector(const ezVec3Template<Type>& v)
{
  Element(3, 0) = v.x;
  Element(3, 1) = v.y;
  Element(3, 2) = v.z;
}

template<typename Type>
void ezMat4Template<Type>::SetRotationalPart(const ezMat3Template<Type>& Rotation)
{
  for (ezUInt32 col = 0; col < 3; ++col)
  {
    for (ezUInt32 row = 0; row < 3; ++row)
    {
      Element(col, row) = Rotation.Element(col, row);
    }
  }
}

template<typename Type>
const ezMat3Template<Type> ezMat4Template<Type>::GetRotationalPart() const
{
  ezMat3Template<Type> r;

  for (ezUInt32 col = 0; col < 3; ++col)
  {
    for (ezUInt32 row = 0; row < 3; ++row)
    {
      r.Element(col, row) = Element(col, row);
    }
  }

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
void ezMat4Template<Type>::operator*= (Type f)
{
  for (ezInt32 i = 0; i < 16; ++i)
    m_fElementsCM[i] *= f;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
void ezMat4Template<Type>::operator/= (Type f)
{
  const Type fInv = ezMath::Invert(f);

  operator*= (fInv);
}

template<typename Type>
const ezMat4Template<Type> operator* (const ezMat4Template<Type>& m1, const ezMat4Template<Type>& m2)
{
  ezMat4Template<Type> r;
  for (ezInt32 i = 0; i < 4; ++i)
  {
    r.Element(0, i) = m1.Element(0, i) * m2.Element(0, 0) + m1.Element(1, i) * m2.Element(0, 1) + m1.Element(2, i) * m2.Element(0, 2) + m1.Element(3, i) * m2.Element(0, 3);
    r.Element(1, i) = m1.Element(0, i) * m2.Element(1, 0) + m1.Element(1, i) * m2.Element(1, 1) + m1.Element(2, i) * m2.Element(1, 2) + m1.Element(3, i) * m2.Element(1, 3);
    r.Element(2, i) = m1.Element(0, i) * m2.Element(2, 0) + m1.Element(1, i) * m2.Element(2, 1) + m1.Element(2, i) * m2.Element(2, 2) + m1.Element(3, i) * m2.Element(2, 3);
    r.Element(3, i) = m1.Element(0, i) * m2.Element(3, 0) + m1.Element(1, i) * m2.Element(3, 1) + m1.Element(2, i) * m2.Element(3, 2) + m1.Element(3, i) * m2.Element(3, 3);
  }

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator* (const ezMat4Template<Type>& m, const ezVec3Template<Type>& v)
{
  return m.TransformPosition(v);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec4Template<Type> operator* (const ezMat4Template<Type>& m, const ezVec4Template<Type>& v)
{
  return m.Transform(v);
}




// *** Stuff needed for matrix inversion ***

template<typename Type>
EZ_FORCE_INLINE Type GetDeterminantOf3x3SubMatrix(const ezMat4Template<Type>& m, ezInt32 i, ezInt32 j)
{
  const ezInt32 si0 = 0 + ((i <= 0) ? 1 : 0);
  const ezInt32 si1 = 1 + ((i <= 1) ? 1 : 0);
  const ezInt32 si2 = 2 + ((i <= 2) ? 1 : 0);

  const ezInt32 sj0 = 0 + ((j <= 0) ? 1 : 0);
  const ezInt32 sj1 = 1 + ((j <= 1) ? 1 : 0);
  const ezInt32 sj2 = 2 + ((j <= 2) ? 1 : 0);

  Type fDet2 = ((m.Element(sj0, si0) * m.Element(sj1, si1) * m.Element(sj2, si2) +
                  m.Element(sj1, si0) * m.Element(sj2, si1) * m.Element(sj0, si2) +
                  m.Element(sj2, si0) * m.Element(sj0, si1) * m.Element(sj1, si2))-
                 (m.Element(sj0, si2) * m.Element(sj1, si1) * m.Element(sj2, si0) +
                  m.Element(sj1, si2) * m.Element(sj2, si1) * m.Element(sj0, si0) +
                  m.Element(sj2, si2) * m.Element(sj0, si1) * m.Element(sj1, si0)));

  return fDet2;
}

template<typename Type>
EZ_FORCE_INLINE Type GetDeterminantOf4x4Matrix(const ezMat4Template<Type>& m)
{
  Type det = 0.0;

  det +=  m.Element(0, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 0);
  det += -m.Element(1, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 1);
  det +=  m.Element(2, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 2);
  det += -m.Element(3, 0) * GetDeterminantOf3x3SubMatrix(m, 0, 3);

  return det;
}


// *** free functions ***

template<typename Type>
EZ_FORCE_INLINE const ezMat4Template<Type> operator* (Type f, const ezMat4Template<Type>& m1)
{
  return operator* (m1, f);
}

template<typename Type>
const ezMat4Template<Type> operator* (const ezMat4Template<Type>& m1, Type f)
{
  ezMat4Template<Type> r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
const ezMat4Template<Type> operator/ (const ezMat4Template<Type>& m1, Type f)
{
  return operator* (m1, ezMath::Invert(f));
}

template<typename Type>
const ezMat4Template<Type> operator+ (const ezMat4Template<Type>& m1, const ezMat4Template<Type>& m2)
{
  ezMat4Template<Type> r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
const ezMat4Template<Type> operator- (const ezMat4Template<Type>& m1, const ezMat4Template<Type>& m2)
{
  ezMat4Template<Type> r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  EZ_NAN_ASSERT(&r);
  return r;
}

template<typename Type>
bool ezMat4Template<Type>::IsIdentical(const ezMat4Template<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

template<typename Type>
bool ezMat4Template<Type>::IsEqual(const ezMat4Template<Type>& rhs, Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  EZ_ASSERT_DEBUG(fEpsilon >= 0.0f, "Epsilon may not be negative.");

  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (!ezMath::IsEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezMat4Template<Type>& lhs, const ezMat4Template<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezMat4Template<Type>& lhs, const ezMat4Template<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template<typename Type>
bool ezMat4Template<Type>::IsZero(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (!ezMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

template<typename Type>
bool ezMat4Template<Type>::IsIdentity(Type fEpsilon) const
{
  EZ_NAN_ASSERT(this);

  if (!ezMath::IsEqual(Element(0, 0), (Type) 1, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(0, 1), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(0, 2), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(0, 3), (Type) 0, fEpsilon)) return false;

  if (!ezMath::IsEqual(Element(1, 0), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(1, 1), (Type) 1, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(1, 2), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(1, 3), (Type) 0, fEpsilon)) return false;

  if (!ezMath::IsEqual(Element(2, 0), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(2, 1), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(2, 2), (Type) 1, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(2, 3), (Type) 0, fEpsilon)) return false;

  if (!ezMath::IsEqual(Element(3, 0), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(3, 1), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(3, 2), (Type) 0, fEpsilon)) return false;
  if (!ezMath::IsEqual(Element(3, 3), (Type) 1, fEpsilon)) return false;

  return true;
}

template<typename Type>
bool ezMat4Template<Type>::IsValid() const
{
  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (!ezMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}

template<typename Type>
bool ezMat4Template<Type>::IsNaN() const
{
  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (ezMath::IsNaN(m_fElementsCM[i]))
      return true;
  }

  return false;
}

template<typename Type>
const ezVec3Template<Type> ezMat4Template<Type>::GetScalingFactors() const
{
  ezVec3Template<Type> v;

  v.x = ezVec3Template<Type>(Element(0, 0), Element(0, 1), Element(0, 2)).GetLength ();
  v.y = ezVec3Template<Type>(Element(1, 0), Element(1, 1), Element(1, 2)).GetLength ();
  v.z = ezVec3Template<Type>(Element(2, 0), Element(2, 1), Element(2, 2)).GetLength ();

  EZ_NAN_ASSERT(&v);
  return v;
}

template<typename Type>
ezResult ezMat4Template<Type>::SetScalingFactors(const ezVec3Template<Type>& vXYZ, Type fEpsilon /* = ezMath::BasicType<Type>::DefaultEpsilon() */)
{
  ezVec3Template<Type> tx (Element(0, 0), Element(0, 1), Element(0, 2));
  ezVec3Template<Type> ty (Element(1, 0), Element(1, 1), Element(1, 2));
  ezVec3Template<Type> tz (Element(2, 0), Element(2, 1), Element(2, 2));

  if (tx.SetLength (vXYZ.x, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;
  if (ty.SetLength (vXYZ.y, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;
  if (tz.SetLength (vXYZ.z, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;


  Element(0, 0) = tx.x; Element(0, 1) = tx.y; Element(0, 2) = tx.z;
  Element(1, 0) = ty.x; Element(1, 1) = ty.y; Element(1, 2) = ty.z;
  Element(2, 0) = tz.x; Element(2, 1) = tz.y; Element(2, 2) = tz.z;

  return EZ_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>

