#pragma once

EZ_ALWAYS_INLINE ezSimdMat4d::ezSimdMat4d() = default;

inline ezSimdMat4d ezSimdMat4d::MakeFromValues(double f1r1, double f2r1, double f3r1, double f4r1, double f1r2, double f2r2, double f3r2, double f4r2, double f1r3,
  double f2r3, double f3r3, double f4r3, double f1r4, double f2r4, double f3r4, double f4r4)
{
  ezSimdMat4d res;
  res.m_col0.Set(f1r1, f1r2, f1r3, f1r4);
  res.m_col1.Set(f2r1, f2r2, f2r3, f2r4);
  res.m_col2.Set(f3r1, f3r2, f3r3, f3r4);
  res.m_col3.Set(f4r1, f4r2, f4r3, f4r4);
  return res;
}

inline ezSimdMat4d ezSimdMat4d::MakeFromColumns(const ezSimdVec4d& vCol0, const ezSimdVec4d& vCol1, const ezSimdVec4d& vCol2, const ezSimdVec4d& vCol3)
{
  ezSimdMat4d res;
  res.m_col0 = vCol0;
  res.m_col1 = vCol1;
  res.m_col2 = vCol2;
  res.m_col3 = vCol3;
  return res;
}

inline ezSimdMat4d ezSimdMat4d::MakeFromRowMajorArray(const double* const pData)
{
  ezSimdMat4d res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  res.Transpose();
  return res;
}

inline ezSimdMat4d ezSimdMat4d::MakeFromColumnMajorArray(const double* const pData)
{
  ezSimdMat4d res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  return res;
}

inline void ezSimdMat4d::GetAsArray(double* out_pData, ezMatrixLayout::Enum layout) const
{
  ezSimdMat4d tmp = *this;

  if (layout == ezMatrixLayout::RowMajor)
  {
    tmp.Transpose();
  }

  tmp.m_col0.Store<4>(out_pData + 0);
  tmp.m_col1.Store<4>(out_pData + 4);
  tmp.m_col2.Store<4>(out_pData + 8);
  tmp.m_col3.Store<4>(out_pData + 12);
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdMat4d::MakeZero()
{
  ezSimdMat4d res;
  res.m_col0.SetZero();
  res.m_col1.SetZero();
  res.m_col2.SetZero();
  res.m_col3.SetZero();
  return res;
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdMat4d::MakeIdentity()
{
  ezSimdMat4d res;
  res.m_col0.Set(1, 0, 0, 0);
  res.m_col1.Set(0, 1, 0, 0);
  res.m_col2.Set(0, 0, 1, 0);
  res.m_col3.Set(0, 0, 0, 1);
  return res;
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdMat4d::GetTranspose() const
{
  ezSimdMat4d result = *this;
  result.Transpose();
  return result;
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdMat4d::GetInverse(const ezSimdDouble& fEpsilon) const
{
  ezSimdMat4d result = *this;
  result.Invert(fEpsilon).IgnoreResult();
  return result;
}

inline bool ezSimdMat4d::IsEqual(const ezSimdMat4d& rhs, const ezSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(rhs.m_col0, fEpsilon) && m_col1.IsEqual(rhs.m_col1, fEpsilon) && m_col2.IsEqual(rhs.m_col2, fEpsilon) &&
          m_col3.IsEqual(rhs.m_col3, fEpsilon))
    .AllSet<4>();
}

inline bool ezSimdMat4d::IsIdentity(const ezSimdDouble& fEpsilon) const
{
  return (m_col0.IsEqual(ezSimdVec4d(1, 0, 0, 0), fEpsilon) && m_col1.IsEqual(ezSimdVec4d(0, 1, 0, 0), fEpsilon) &&
          m_col2.IsEqual(ezSimdVec4d(0, 0, 1, 0), fEpsilon) && m_col3.IsEqual(ezSimdVec4d(0, 0, 0, 1), fEpsilon))
    .AllSet<4>();
}

inline bool ezSimdMat4d::IsValid() const
{
  return m_col0.IsValid<4>() && m_col1.IsValid<4>() && m_col2.IsValid<4>() && m_col3.IsValid<4>();
}

inline bool ezSimdMat4d::IsNaN() const
{
  return m_col0.IsNaN<4>() || m_col1.IsNaN<4>() || m_col2.IsNaN<4>() || m_col3.IsNaN<4>();
}

EZ_ALWAYS_INLINE void ezSimdMat4d::SetRows(const ezSimdVec4d& vRow0, const ezSimdVec4d& vRow1, const ezSimdVec4d& vRow2, const ezSimdVec4d& vRow3)
{
  m_col0 = vRow0;
  m_col1 = vRow1;
  m_col2 = vRow2;
  m_col3 = vRow3;

  Transpose();
}

EZ_ALWAYS_INLINE void ezSimdMat4d::GetRows(ezSimdVec4d& ref_vRow0, ezSimdVec4d& ref_vRow1, ezSimdVec4d& ref_vRow2, ezSimdVec4d& ref_vRow3) const
{
  ezSimdMat4d tmp = *this;
  tmp.Transpose();

  ref_vRow0 = tmp.m_col0;
  ref_vRow1 = tmp.m_col1;
  ref_vRow2 = tmp.m_col2;
  ref_vRow3 = tmp.m_col3;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdMat4d::TransformPosition(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();
  result += m_col3;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdMat4d::TransformDirection(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();

  return result;
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdMat4d::operator*(const ezSimdMat4d& rhs) const
{
  ezSimdMat4d result;

  result.m_col0 = m_col0 * rhs.m_col0.x();
  result.m_col0 += m_col1 * rhs.m_col0.y();
  result.m_col0 += m_col2 * rhs.m_col0.z();
  result.m_col0 += m_col3 * rhs.m_col0.w();

  result.m_col1 = m_col0 * rhs.m_col1.x();
  result.m_col1 += m_col1 * rhs.m_col1.y();
  result.m_col1 += m_col2 * rhs.m_col1.z();
  result.m_col1 += m_col3 * rhs.m_col1.w();

  result.m_col2 = m_col0 * rhs.m_col2.x();
  result.m_col2 += m_col1 * rhs.m_col2.y();
  result.m_col2 += m_col2 * rhs.m_col2.z();
  result.m_col2 += m_col3 * rhs.m_col2.w();

  result.m_col3 = m_col0 * rhs.m_col3.x();
  result.m_col3 += m_col1 * rhs.m_col3.y();
  result.m_col3 += m_col2 * rhs.m_col3.z();
  result.m_col3 += m_col3 * rhs.m_col3.w();

  return result;
}

EZ_ALWAYS_INLINE void ezSimdMat4d::operator*=(const ezSimdMat4d& rhs)
{
  *this = *this * rhs;
}

EZ_ALWAYS_INLINE bool ezSimdMat4d::operator==(const ezSimdMat4d& other) const
{
  return (m_col0 == other.m_col0 && m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3).AllSet<4>();
}

EZ_ALWAYS_INLINE bool ezSimdMat4d::operator!=(const ezSimdMat4d& other) const
{
  return !(*this == other);
}

EZ_ALWAYS_INLINE ezSimdMat4d MultiplyAffine(const ezSimdMat4d& lhs, const ezSimdMat4d& rhs)
{
  ezSimdMat4d result;

  result.m_col0 = lhs.m_col0 * rhs.m_col0.x();
  result.m_col0 += lhs.m_col1 * rhs.m_col0.y();
  result.m_col0 += lhs.m_col2 * rhs.m_col0.z();

  result.m_col1 = lhs.m_col0 * rhs.m_col1.x();
  result.m_col1 += lhs.m_col1 * rhs.m_col1.y();
  result.m_col1 += lhs.m_col2 * rhs.m_col1.z();

  result.m_col2 = lhs.m_col0 * rhs.m_col2.x();
  result.m_col2 += lhs.m_col1 * rhs.m_col2.y();
  result.m_col2 += lhs.m_col2 * rhs.m_col2.z();

  result.m_col3 = lhs.m_col0 * rhs.m_col3.x();
  result.m_col3 += lhs.m_col1 * rhs.m_col3.y();
  result.m_col3 += lhs.m_col2 * rhs.m_col3.z();
  result.m_col3 += lhs.m_col3;

  return result;
}
