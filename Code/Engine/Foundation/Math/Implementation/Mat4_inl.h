#pragma once

#include <Foundation/Math/Mat3.h>

inline ezMat4::ezMat4()
{
#if EZ_COMPILE_FOR_DEBUG
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  SetElements(fNaN, fNaN, fNaN, fNaN,
            fNaN, fNaN, fNaN, fNaN,
            fNaN, fNaN, fNaN, fNaN,
            fNaN, fNaN, fNaN, fNaN);
#endif
}

inline ezMat4::ezMat4(const float* const pData, ezMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

inline ezMat4::ezMat4(float c1r1, float c2r1, float c3r1, float c4r1,
                               float c1r2, float c2r2, float c3r2, float c4r2,
                               float c1r3, float c2r3, float c3r3, float c4r3,
                               float c1r4, float c2r4, float c3r4, float c4r4,
                               ezMatrixLayout::Enum layout)
{
  SetElements(c1r1, c2r1, c3r1, c4r1,
              c1r2, c2r2, c3r2, c4r2,
              c1r3, c2r3, c3r3, c4r3,
              c1r4, c2r4, c3r4, c4r4,
              layout);
}

inline ezMat4::ezMat4(const ezMat3& Rotation, const ezVec3& vTranslation)
{
  SetTransformationMatrix(Rotation, vTranslation);
}

inline const ezMat4 ezMat4::IdentityMatrix()
{
  return ezMat4(1, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1);
}

inline const ezMat4 ezMat4::ZeroMatrix()
{
  return ezMat4(0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0,
                0, 0, 0, 0);
}

inline void ezMat4::SetFromArray(const float* EZ_RESTRICT const pData, ezMatrixLayout::Enum layout)
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(m_fElementsCM, pData, 16);
  }
  else
  {
    for (int i = 0; i < 4; ++i)
    {
      m_fColumn[0][i] = pData[i * 4 + 0];
      m_fColumn[1][i] = pData[i * 4 + 1];
      m_fColumn[2][i] = pData[i * 4 + 2];
      m_fColumn[3][i] = pData[i * 4 + 3];
    }
  }
}

inline void ezMat4::SetTransformationMatrix(const ezMat3& Rotation, const ezVec3& vTranslation)
{
  SetRotationalPart(Rotation);
  SetTranslationVector(vTranslation);
  SetRow(3, ezVec4(0, 0, 0, 1));
}

inline void ezMat4::GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(out_pData, m_fElementsCM, 16);
  }
  else
  {
    for (int i = 0; i < 4; ++i)
    {
      out_pData[i * 4 + 0] = m_fColumn[0][i];
      out_pData[i * 4 + 1] = m_fColumn[1][i];
      out_pData[i * 4 + 2] = m_fColumn[2][i];
      out_pData[i * 4 + 3] = m_fColumn[3][i];
    }
  }
}

inline void ezMat4::SetElements(float c1r1, float c2r1, float c3r1, float c4r1,
                                float c1r2, float c2r2, float c3r2, float c4r2,
                                float c1r3, float c2r3, float c3r3, float c4r3,
                                float c1r4, float c2r4, float c3r4, float c4r4,
                                ezMatrixLayout::Enum layout)
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    m_fColumn[0][0] = c1r1;	m_fColumn[0][1] = c2r1;	m_fColumn[0][2] = c3r1;	m_fColumn[0][3] = c4r1;
    m_fColumn[1][0] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[1][2] = c3r2;	m_fColumn[1][3] = c4r2;
    m_fColumn[2][0] = c1r3;	m_fColumn[2][1] = c2r3;	m_fColumn[2][2] = c3r3;	m_fColumn[2][3] = c4r3;
    m_fColumn[3][0] = c1r4;	m_fColumn[3][1] = c2r4;	m_fColumn[3][2] = c3r4;	m_fColumn[3][3] = c4r4;
  }
  else
  {
    m_fColumn[0][0] = c1r1;	m_fColumn[1][0] = c2r1;	m_fColumn[2][0] = c3r1;	m_fColumn[3][0] = c4r1;
    m_fColumn[0][1] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[2][1] = c3r2;	m_fColumn[3][1] = c4r2;
    m_fColumn[0][2] = c1r3;	m_fColumn[1][2] = c2r3;	m_fColumn[2][2] = c3r3;	m_fColumn[3][2] = c4r3;
    m_fColumn[0][3] = c1r4;	m_fColumn[1][3] = c2r4;	m_fColumn[2][3] = c3r4;	m_fColumn[3][3] = c4r4;
  }
}

inline void ezMat4::SetZero()
{
  SetElements(0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0);
}

inline void ezMat4::SetIdentity()
{
  SetElements(1, 0, 0, 0,
              0, 1, 0, 0,
              0, 0, 1, 0,
              0, 0, 0, 1);
}

inline void ezMat4::SetTranslationMatrix(const ezVec3& vTranslation)
{
  SetElements(1, 0, 0, vTranslation.x,
              0, 1, 0, vTranslation.y,
              0, 0, 1, vTranslation.z,
              0, 0, 0, 1);
}

inline void ezMat4::SetScalingMatrix(const ezVec3& s)
{
  SetElements(s.x,  0,   0,   0,
               0,  s.y,  0,   0,
               0,   0,  s.z,  0,
               0,   0,   0,   1);
}

inline void ezMat4::SetRotationMatrixX(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);

  SetElements(1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, fCos,-fSin, 0.0f,
            0.0f, fSin, fCos, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

inline void ezMat4::SetRotationMatrixY(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);


  SetElements(fCos, 0.0f, fSin, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
           -fSin, 0.0f, fCos, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

inline void ezMat4::SetRotationMatrixZ(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);

  SetElements(fCos,-fSin, 0.0f, 0.0f,
            fSin, fCos, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);
}

inline void ezMat4::Transpose()
{
  ezMath::Swap(m_fColumn[0][1], m_fColumn[1][0]);
  ezMath::Swap(m_fColumn[0][2], m_fColumn[2][0]);
  ezMath::Swap(m_fColumn[0][3], m_fColumn[3][0]);
  ezMath::Swap(m_fColumn[1][2], m_fColumn[2][1]);
  ezMath::Swap(m_fColumn[1][3], m_fColumn[3][1]);
  ezMath::Swap(m_fColumn[2][3], m_fColumn[3][2]);
}

inline const ezMat4 ezMat4::GetTranspose() const
{
  return ezMat4(m_fElementsCM, ezMatrixLayout::RowMajor);
}


inline const ezMat4 ezMat4::GetInverse() const
{
  ezMat4 Inverse = *this;
  Inverse.Invert();
  return Inverse;
}

inline ezVec4 ezMat4::GetRow(ezUInt32 uiRow) const
{
  EZ_ASSERT(uiRow <= 3, "Invalid Row Index %d", uiRow);

  ezVec4 r;
  r.x = m_fColumn[0][uiRow];
  r.y = m_fColumn[1][uiRow];
  r.z = m_fColumn[2][uiRow];
  r.w = m_fColumn[3][uiRow];

  return r;
}

inline void ezMat4::SetRow(ezUInt32 uiRow, const ezVec4& row)
{
  EZ_ASSERT(uiRow <= 3, "Invalid Row Index %d", uiRow);

  m_fColumn[0][uiRow] = row.x;
  m_fColumn[1][uiRow] = row.y;
  m_fColumn[2][uiRow] = row.z;
  m_fColumn[3][uiRow] = row.w;
}

inline ezVec4 ezMat4::GetColumn(ezUInt32 uiColumn) const
{
  EZ_ASSERT(uiColumn <= 3, "Invalid Column Index %d", uiColumn);

  ezVec4 r;
  r.x = m_fColumn[uiColumn][0];
  r.y = m_fColumn[uiColumn][1];
  r.z = m_fColumn[uiColumn][2];
  r.w = m_fColumn[uiColumn][3];

  return r;
}

inline void ezMat4::SetColumn(ezUInt32 uiColumn, const ezVec4& column)
{
  EZ_ASSERT(uiColumn <= 3, "Invalid Column Index %d", uiColumn);

  m_fColumn[uiColumn][0] = column.x;
  m_fColumn[uiColumn][1] = column.y;
  m_fColumn[uiColumn][2] = column.z;
  m_fColumn[uiColumn][3] = column.w;
}

inline ezVec4 ezMat4::GetDiagonal() const
{
  return ezVec4(m_fColumn[0][0], m_fColumn[1][1], m_fColumn[2][2], m_fColumn[3][3]);
}

inline void ezMat4::SetDiagonal(const ezVec4& diag)
{
  m_fColumn[0][0] = diag.x;
  m_fColumn[1][1] = diag.y;
  m_fColumn[2][2] = diag.z;
  m_fColumn[3][3] = diag.w;
}

inline const ezVec3 ezMat4::TransformPosition(const ezVec3& v) const
{
  ezVec3 r;
  r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z + m_fColumn[3][0];
  r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z + m_fColumn[3][1];
  r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z + m_fColumn[3][2];
  return r;
}

inline void ezMat4::TransformPosition(ezVec3* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(inout_v != NULL, "Array must not be NULL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "Data must not overlap.");

  ezVec3* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformPosition(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

inline const ezVec3 ezMat4::TransformDirection(const ezVec3& v) const
{
  ezVec3 r;
  r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z;
  r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z;
  r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z;
  return r;
}

inline void ezMat4::TransformDirection(ezVec3* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(inout_v != NULL, "Array must not be NULL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "Data must not overlap.");

  ezVec3* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = TransformDirection(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

inline const ezVec4 ezMat4::Transform(const ezVec4& v) const
{
  ezVec4 r;
  r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z + m_fColumn[3][0] * v.w;
  r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z + m_fColumn[3][1] * v.w;
  r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z + m_fColumn[3][2] * v.w;
  r.w = m_fColumn[0][3] * v.x + m_fColumn[1][3] * v.y + m_fColumn[2][3] * v.z + m_fColumn[3][3] * v.w;
  return r;
}

inline void ezMat4::Transform(ezVec4* inout_v, ezUInt32 uiNumVectors, ezUInt32 uiStride /* = sizeof(ezVec4) */) const
{
  EZ_ASSERT(inout_v != NULL, "Array must not be NULL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec4), "Data must not overlap.");

  ezVec4* pCur = inout_v;

  for (ezUInt32 i = 0; i < uiNumVectors; ++i)
  {
    *pCur = Transform(*pCur);
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

EZ_FORCE_INLINE const ezVec3 ezMat4::GetTranslationVector() const
{
  return ezVec3(m_fColumn[3][0], m_fColumn[3][1], m_fColumn[3][2]);
}

EZ_FORCE_INLINE void ezMat4::SetTranslationVector(const ezVec3& v)
{
  m_fColumn[3][0] = v.x;
  m_fColumn[3][1] = v.y;
  m_fColumn[3][2] = v.z;
}

inline void ezMat4::SetRotationalPart(const ezMat3& Rotation)
{
  for (ezUInt32 col = 0; col < 3; ++col)
  {
    for (ezUInt32 row = 0; row < 3; ++row)
    {
      m_fColumn[col][row] = Rotation.m_fColumn[col][row];
    }
  }
}

inline const ezMat3 ezMat4::GetRotationalPart() const
{
  ezMat3 r;

  for (ezUInt32 col = 0; col < 3; ++col)
  {
    for (ezUInt32 row = 0; row < 3; ++row)
    {
      r.m_fColumn[col][row] = m_fColumn[col][row];
    }
  }

  return r;
}

inline void ezMat4::operator*= (float f)
{
  for (ezInt32 i = 0; i < 16; ++i)
    m_fElementsCM[i] *= f;
}

inline void ezMat4::operator/= (float f)
{
  const float fInv = ezMath::Invert(f);

  operator*= (fInv);
}

inline const ezMat4 operator* (const ezMat4& m1, const ezMat4& m2)
{
  ezMat4 r;
  for (ezInt32 i = 0; i < 4; ++i)
  {
    r.m_fColumn[0][i] = m1.m_fColumn[0][i] * m2.m_fColumn[0][0] + m1.m_fColumn[1][i] * m2.m_fColumn[0][1] + m1.m_fColumn[2][i] * m2.m_fColumn[0][2] + m1.m_fColumn[3][i] * m2.m_fColumn[0][3];
    r.m_fColumn[1][i] = m1.m_fColumn[0][i] * m2.m_fColumn[1][0] + m1.m_fColumn[1][i] * m2.m_fColumn[1][1] + m1.m_fColumn[2][i] * m2.m_fColumn[1][2] + m1.m_fColumn[3][i] * m2.m_fColumn[1][3];
    r.m_fColumn[2][i] = m1.m_fColumn[0][i] * m2.m_fColumn[2][0] + m1.m_fColumn[1][i] * m2.m_fColumn[2][1] + m1.m_fColumn[2][i] * m2.m_fColumn[2][2] + m1.m_fColumn[3][i] * m2.m_fColumn[2][3];
    r.m_fColumn[3][i] = m1.m_fColumn[0][i] * m2.m_fColumn[3][0] + m1.m_fColumn[1][i] * m2.m_fColumn[3][1] + m1.m_fColumn[2][i] * m2.m_fColumn[3][2] + m1.m_fColumn[3][i] * m2.m_fColumn[3][3];
  }
  return r;
}

EZ_FORCE_INLINE const ezVec3 operator* (const ezMat4& m, const ezVec3& v)
{
  return m.TransformPosition(v);
}

EZ_FORCE_INLINE const ezVec4 operator* (const ezMat4& m, const ezVec4& v)
{
  return m.Transform(v);
}




// *** Stuff needed for matrix inversion ***

EZ_FORCE_INLINE float GetDeterminantOf3x3SubMatrix(const ezMat4& m, ezInt32 i, ezInt32 j)
{
  const ezInt32 si0 = 0 + ((i <= 0) ? 1 : 0);
  const ezInt32 si1 = 1 + ((i <= 1) ? 1 : 0);
  const ezInt32 si2 = 2 + ((i <= 2) ? 1 : 0);

  const ezInt32 sj0 = 0 + ((j <= 0) ? 1 : 0);
  const ezInt32 sj1 = 1 + ((j <= 1) ? 1 : 0);
  const ezInt32 sj2 = 2 + ((j <= 2) ? 1 : 0);

  float fDet2 = ((m.m_fColumn[sj0][si0] * m.m_fColumn[sj1][si1] * m.m_fColumn[sj2][si2] +
                  m.m_fColumn[sj1][si0] * m.m_fColumn[sj2][si1] * m.m_fColumn[sj0][si2] +
                  m.m_fColumn[sj2][si0] * m.m_fColumn[sj0][si1] * m.m_fColumn[sj1][si2])-
                  (m.m_fColumn[sj0][si2] * m.m_fColumn[sj1][si1] * m.m_fColumn[sj2][si0] +
                  m.m_fColumn[sj1][si2] * m.m_fColumn[sj2][si1] * m.m_fColumn[sj0][si0] +
                  m.m_fColumn[sj2][si2] * m.m_fColumn[sj0][si1] * m.m_fColumn[sj1][si0]));

  return fDet2;
}

EZ_FORCE_INLINE float GetDeterminantOf4x4Matrix(const ezMat4& m)
{
  float det = 0.0;

  det +=  m.m_fColumn[0][0] * GetDeterminantOf3x3SubMatrix(m, 0, 0);
  det += -m.m_fColumn[1][0] * GetDeterminantOf3x3SubMatrix(m, 0, 1);
  det +=  m.m_fColumn[2][0] * GetDeterminantOf3x3SubMatrix(m, 0, 2);
  det += -m.m_fColumn[3][0] * GetDeterminantOf3x3SubMatrix(m, 0, 3);

  return det;
}


// *** free functions ***

EZ_FORCE_INLINE const ezMat4 operator* (float f, const ezMat4& m1)
{
  return operator* (m1, f);
}

inline const ezMat4 operator* (const ezMat4& m1, float f)
{
  ezMat4 r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  return r;
}

inline const ezMat4 operator/ (const ezMat4& m1, float f)
{
  return operator* (m1, ezMath::Invert(f));
}

inline const ezMat4 operator+ (const ezMat4& m1, const ezMat4& m2)
{
  ezMat4 r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  return r;
}

inline const ezMat4 operator- (const ezMat4& m1, const ezMat4& m2)
{
  ezMat4 r;

  for (ezUInt32 i = 0; i < 16; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  return r;
}

inline bool ezMat4::IsIdentical(const ezMat4& rhs) const
{
  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

inline bool ezMat4::IsEqual(const ezMat4& rhs, float fEpsilon) const
{
  EZ_ASSERT(fEpsilon >= 0.0f, "Epsilon may not be negativ.");

  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (ezMath::IsFloatEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

EZ_FORCE_INLINE bool operator== (const ezMat4& lhs, const ezMat4& rhs)
{
  return lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE bool operator!= (const ezMat4& lhs, const ezMat4& rhs)
{
  return !lhs.IsIdentical(rhs);
}

inline bool ezMat4::IsZero(float fEpsilon) const
{
  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (!ezMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

inline bool ezMat4::IsIdentity(float fEpsilon) const
{
  if (!ezMath::IsFloatEqual(m_fColumn[0][0], 1.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[0][1], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[0][2], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[0][3], 0.0f, fEpsilon)) return false;

  if (!ezMath::IsFloatEqual(m_fColumn[1][0], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[1][1], 1.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[1][2], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[1][3], 0.0f, fEpsilon)) return false;

  if (!ezMath::IsFloatEqual(m_fColumn[2][0], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[2][1], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[2][2], 1.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[2][3], 0.0f, fEpsilon)) return false;

  if (!ezMath::IsFloatEqual(m_fColumn[3][0], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[3][1], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[3][2], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[3][3], 1.0f, fEpsilon)) return false;

  return true;
}

inline bool ezMat4::IsValid() const
{
  for (ezUInt32 i = 0; i < 16; ++i)
  {
    if (!ezMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;

}

inline const ezVec3 ezMat4::GetScalingFactors() const
{
  ezVec3 v;

  v.x = ezVec3(m_fColumn[0][0], m_fColumn[0][1], m_fColumn[0][2]).GetLength ();
  v.y = ezVec3(m_fColumn[1][0], m_fColumn[1][1], m_fColumn[1][2]).GetLength ();
  v.z = ezVec3(m_fColumn[2][0], m_fColumn[2][1], m_fColumn[2][2]).GetLength ();

  return v;
}

inline ezResult ezMat4::SetScalingFactors(const ezVec3& vXYZ, float fEpsilon /* = ezMath_DefaultEpsilon */)
{
  ezVec3 tx (m_fColumn[0][0], m_fColumn[0][1], m_fColumn[0][2]);
  ezVec3 ty (m_fColumn[1][0], m_fColumn[1][1], m_fColumn[1][2]);
  ezVec3 tz (m_fColumn[2][0], m_fColumn[2][1], m_fColumn[2][2]);

  if (tx.SetLength (vXYZ.x, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;
  if (ty.SetLength (vXYZ.y, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;
  if (tz.SetLength (vXYZ.z, fEpsilon) == EZ_FAILURE) return EZ_FAILURE;


  m_fColumn[0][0] = tx.x; m_fColumn[0][1] = tx.y; m_fColumn[0][2] = tx.z;
  m_fColumn[1][0] = ty.x; m_fColumn[1][1] = ty.y; m_fColumn[1][2] = ty.z;
  m_fColumn[2][0] = tz.x; m_fColumn[2][1] = tz.y; m_fColumn[2][2] = tz.z;

  return EZ_SUCCESS;
}

