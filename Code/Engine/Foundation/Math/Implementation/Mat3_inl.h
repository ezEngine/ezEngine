#pragma once

inline ezMat3::ezMat3()
{
#if EZ_COMPILE_FOR_DEBUG
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float fNaN = ezMath::NaN();
  SetElements(fNaN, fNaN, fNaN,
            fNaN, fNaN, fNaN,
            fNaN, fNaN, fNaN);
#endif
}

inline ezMat3::ezMat3(const float* const pData, ezMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

inline ezMat3::ezMat3(float c1r1, float c2r1, float c3r1,
                               float c1r2, float c2r2, float c3r2,
                               float c1r3, float c2r3, float c3r3,
                               ezMatrixLayout::Enum layout)
{
  SetElements(c1r1, c2r1, c3r1,
              c1r2, c2r2, c3r2,
              c1r3, c2r3, c3r3,
              layout);
}

EZ_FORCE_INLINE const ezMat3 ezMat3::IdentityMatrix()
{
  return ezMat3(1, 0, 0,
                0, 1, 0,
                0, 0, 1);
}

EZ_FORCE_INLINE const ezMat3 ezMat3::ZeroMatrix()
{
  return ezMat3(0, 0, 0,
                0, 0, 0,
                0, 0, 0);
}

inline void ezMat3::SetFromArray(const float* EZ_RESTRICT const pData, ezMatrixLayout::Enum layout)
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(m_fElementsCM, pData, 9);
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      m_fColumn[0][i] = pData[i * 3 + 0];
      m_fColumn[1][i] = pData[i * 3 + 1];
      m_fColumn[2][i] = pData[i * 3 + 2];
    }
  }
}

inline void ezMat3::GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(out_pData, m_fElementsCM, 9);
  }
  else
  {
    for (int i = 0; i < 3; ++i)
    {
      out_pData[i * 3 + 0] = m_fColumn[0][i];
      out_pData[i * 3 + 1] = m_fColumn[1][i];
      out_pData[i * 3 + 2] = m_fColumn[2][i];
    }
  }
}

inline void ezMat3::SetElements(float c1r1, float c2r1, float c3r1,
                                float c1r2, float c2r2, float c3r2,
                                float c1r3, float c2r3, float c3r3,
                                ezMatrixLayout::Enum layout)
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    m_fColumn[0][0] = c1r1;	m_fColumn[0][1] = c2r1;	m_fColumn[0][2] = c3r1;
    m_fColumn[1][0] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[1][2] = c3r2;
    m_fColumn[2][0] = c1r3;	m_fColumn[2][1] = c2r3;	m_fColumn[2][2] = c3r3;
  }
  else
  {
    m_fColumn[0][0] = c1r1;	m_fColumn[1][0] = c2r1;	m_fColumn[2][0] = c3r1;
    m_fColumn[0][1] = c1r2;	m_fColumn[1][1] = c2r2;	m_fColumn[2][1] = c3r2;
    m_fColumn[0][2] = c1r3;	m_fColumn[1][2] = c2r3;	m_fColumn[2][2] = c3r3;
  }
}

inline void ezMat3::SetZero()
{
  SetElements(0, 0, 0,
              0, 0, 0,
              0, 0, 0);
}

inline void ezMat3::SetIdentity()
{
  SetElements(1, 0, 0,
              0, 1, 0,
              0, 0, 1);
}

inline void ezMat3::SetScalingMatrix(const ezVec3& s)
{
  SetElements(s.x,  0,   0,
               0,  s.y,  0,
               0,   0,  s.z);
}

inline void ezMat3::SetRotationMatrixX(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);

  SetElements(1.0f, 0.0f, 0.0f,
              0.0f, fCos,-fSin,
              0.0f, fSin, fCos);
}

inline void ezMat3::SetRotationMatrixY(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);


  SetElements(fCos, 0.0f, fSin,
              0.0f, 1.0f, 0.0f,
             -fSin, 0.0f, fCos);
}

inline void ezMat3::SetRotationMatrixZ(float fAngle)
{
  const float fSin = ezMath::SinDeg(fAngle);
  const float fCos = ezMath::CosDeg(fAngle);

  SetElements(fCos,-fSin, 0.0f,
              fSin, fCos, 0.0f,
              0.0f, 0.0f, 1.0f);
}

inline void ezMat3::Transpose()
{
  ezMath::Swap(m_fColumn[0][1], m_fColumn[1][0]);
  ezMath::Swap(m_fColumn[0][2], m_fColumn[2][0]);
  ezMath::Swap(m_fColumn[1][2], m_fColumn[2][1]);
}

inline const ezMat3 ezMat3::GetTranspose() const
{
  return ezMat3(m_fElementsCM, ezMatrixLayout::RowMajor);
}


inline const ezMat3 ezMat3::GetInverse() const
{
  ezMat3 Inverse = *this;
  Inverse.Invert();
  return Inverse;
}

inline ezVec3 ezMat3::GetRow(ezUInt32 uiRow) const
{
  EZ_ASSERT(uiRow <= 2, "Invalid Row Index %d", uiRow);

  ezVec3 r;
  r.x = m_fColumn[0][uiRow];
  r.y = m_fColumn[1][uiRow];
  r.z = m_fColumn[2][uiRow];

  return r;
}

inline void ezMat3::SetRow(ezUInt32 uiRow, const ezVec3& row)
{
  EZ_ASSERT(uiRow <= 2, "Invalid Row Index %d", uiRow);

  m_fColumn[0][uiRow] = row.x;
  m_fColumn[1][uiRow] = row.y;
  m_fColumn[2][uiRow] = row.z;
}

inline ezVec3 ezMat3::GetColumn(ezUInt32 uiColumn) const
{
  EZ_ASSERT(uiColumn <= 2, "Invalid Column Index %d", uiColumn);

  ezVec3 r;
  r.x = m_fColumn[uiColumn][0];
  r.y = m_fColumn[uiColumn][1];
  r.z = m_fColumn[uiColumn][2];

  return r;
}

inline void ezMat3::SetColumn(ezUInt32 uiColumn, const ezVec3& column)
{
  EZ_ASSERT(uiColumn <= 2, "Invalid Column Index %d", uiColumn);

  m_fColumn[uiColumn][0] = column.x;
  m_fColumn[uiColumn][1] = column.y;
  m_fColumn[uiColumn][2] = column.z;
}

inline ezVec3 ezMat3::GetDiagonal() const
{
  return ezVec3(m_fColumn[0][0], m_fColumn[1][1], m_fColumn[2][2]);
}

inline void ezMat3::SetDiagonal(const ezVec3& diag)
{
  m_fColumn[0][0] = diag.x;
  m_fColumn[1][1] = diag.y;
  m_fColumn[2][2] = diag.z;
}

EZ_FORCE_INLINE const ezVec3 ezMat3::TransformDirection(const ezVec3& v) const
{
  ezVec3 r;
  r.x = m_fColumn[0][0] * v.x + m_fColumn[1][0] * v.y + m_fColumn[2][0] * v.z;
  r.y = m_fColumn[0][1] * v.x + m_fColumn[1][1] * v.y + m_fColumn[2][1] * v.z;
  r.z = m_fColumn[0][2] * v.x + m_fColumn[1][2] * v.y + m_fColumn[2][2] * v.z;
  return r;
}

EZ_FORCE_INLINE void ezMat3::operator*= (float f)
{
  for (ezInt32 i = 0; i < 9; ++i)
    m_fElementsCM[i] *= f;
}

EZ_FORCE_INLINE void ezMat3::operator/= (float f)
{
  const float fInv = ezMath::Invert(f);

  operator*= (fInv);
}

inline const ezMat3 operator* (const ezMat3& m1, const ezMat3& m2)
{
  ezMat3 r;
  for (ezInt32 i = 0; i < 3; ++i)
  {
    r.m_fColumn[0][i] = m1.m_fColumn[0][i] * m2.m_fColumn[0][0] + m1.m_fColumn[1][i] * m2.m_fColumn[0][1] + m1.m_fColumn[2][i] * m2.m_fColumn[0][2];
    r.m_fColumn[1][i] = m1.m_fColumn[0][i] * m2.m_fColumn[1][0] + m1.m_fColumn[1][i] * m2.m_fColumn[1][1] + m1.m_fColumn[2][i] * m2.m_fColumn[1][2];
    r.m_fColumn[2][i] = m1.m_fColumn[0][i] * m2.m_fColumn[2][0] + m1.m_fColumn[1][i] * m2.m_fColumn[2][1] + m1.m_fColumn[2][i] * m2.m_fColumn[2][2];
  }
  return r;
}

EZ_FORCE_INLINE const ezVec3 operator* (const ezMat3& m, const ezVec3& v)
{
  return m.TransformDirection(v);
}



// *** free functions ***

EZ_FORCE_INLINE const ezMat3 operator* (float f, const ezMat3& m1)
{
  return operator* (m1, f);
}

inline const ezMat3 operator* (const ezMat3& m1, float f)
{
  ezMat3 r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] * f;

  return r;
}

EZ_FORCE_INLINE const ezMat3 operator/ (const ezMat3& m1, float f)
{
  return operator* (m1, ezMath::Invert(f));
}

inline const ezMat3 operator+ (const ezMat3& m1, const ezMat3& m2)
{
  ezMat3 r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] + m2.m_fElementsCM[i];

  return r;
}

inline const ezMat3 operator- (const ezMat3& m1, const ezMat3& m2)
{
  ezMat3 r;

  for (ezUInt32 i = 0; i < 9; ++i)
    r.m_fElementsCM[i] = m1.m_fElementsCM[i] - m2.m_fElementsCM[i];

  return r;
}

inline bool ezMat3::IsIdentical(const ezMat3& rhs) const
{
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (m_fElementsCM[i] != rhs.m_fElementsCM[i])
      return false;
  }

  return true;
}

inline bool ezMat3::IsEqual(const ezMat3& rhs, float fEpsilon) const
{
  EZ_ASSERT(fEpsilon >= 0.0f, "Epsilon may not be negativ.");

  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (ezMath::IsFloatEqual(m_fElementsCM[i], rhs.m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

EZ_FORCE_INLINE bool operator== (const ezMat3& lhs, const ezMat3& rhs)
{
  return lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE bool operator!= (const ezMat3& lhs, const ezMat3& rhs)
{
  return !lhs.IsIdentical(rhs);
}

inline bool ezMat3::IsZero(float fEpsilon) const
{
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (!ezMath::IsZero(m_fElementsCM[i], fEpsilon))
      return false;
  }

  return true;
}

inline bool ezMat3::IsIdentity(float fEpsilon) const
{
  if (!ezMath::IsFloatEqual(m_fColumn[0][0], 1.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[0][1], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[0][2], 0.0f, fEpsilon)) return false;

  if (!ezMath::IsFloatEqual(m_fColumn[1][0], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[1][1], 1.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[1][2], 0.0f, fEpsilon)) return false;

  if (!ezMath::IsFloatEqual(m_fColumn[2][0], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[2][1], 0.0f, fEpsilon)) return false;
  if (!ezMath::IsFloatEqual(m_fColumn[2][2], 1.0f, fEpsilon)) return false;

  return true;
}

inline bool ezMat3::IsValid() const
{
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    if (!ezMath::IsFinite(m_fElementsCM[i]))
      return false;
  }

  return true;
}



