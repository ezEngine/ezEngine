#pragma once

EZ_ALWAYS_INLINE ezSimdMat4f::ezSimdMat4f() = default;

inline ezSimdMat4f ezSimdMat4f::MakeFromValues(float f1r1, float f2r1, float f3r1, float f4r1, float f1r2, float f2r2, float f3r2, float f4r2, float f1r3,
  float f2r3, float f3r3, float f4r3, float f1r4, float f2r4, float f3r4, float f4r4)
{
  ezSimdMat4f res;
  res.m_col0.Set(f1r1, f1r2, f1r3, f1r4);
  res.m_col1.Set(f2r1, f2r2, f2r3, f2r4);
  res.m_col2.Set(f3r1, f3r2, f3r3, f3r4);
  res.m_col3.Set(f4r1, f4r2, f4r3, f4r4);
  return res;
}

inline ezSimdMat4f ezSimdMat4f::MakeFromColumns(const ezSimdVec4f& vCol0, const ezSimdVec4f& vCol1, const ezSimdVec4f& vCol2, const ezSimdVec4f& vCol3)
{
  ezSimdMat4f res;
  res.m_col0 = vCol0;
  res.m_col1 = vCol1;
  res.m_col2 = vCol2;
  res.m_col3 = vCol3;
  return res;
}

inline ezSimdMat4f ezSimdMat4f::MakeFromRowMajorArray(const float* const pData)
{
  ezSimdMat4f res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  res.Transpose();
  return res;
}

inline ezSimdMat4f ezSimdMat4f::MakeFromColumnMajorArray(const float* const pData)
{
  ezSimdMat4f res;
  res.m_col0.Load<4>(pData + 0);
  res.m_col1.Load<4>(pData + 4);
  res.m_col2.Load<4>(pData + 8);
  res.m_col3.Load<4>(pData + 12);
  return res;
}

inline void ezSimdMat4f::GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const
{
  ezSimdMat4f tmp = *this;

  if (layout == ezMatrixLayout::RowMajor)
  {
    tmp.Transpose();
  }

  tmp.m_col0.Store<4>(out_pData + 0);
  tmp.m_col1.Store<4>(out_pData + 4);
  tmp.m_col2.Store<4>(out_pData + 8);
  tmp.m_col3.Store<4>(out_pData + 12);
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::MakeZero()
{
  ezSimdMat4f res;
  res.m_col0.SetZero();
  res.m_col1.SetZero();
  res.m_col2.SetZero();
  res.m_col3.SetZero();
  return res;
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::MakeIdentity()
{
  ezSimdMat4f res;
  res.m_col0.Set(1, 0, 0, 0);
  res.m_col1.Set(0, 1, 0, 0);
  res.m_col2.Set(0, 0, 1, 0);
  res.m_col3.Set(0, 0, 0, 1);
  return res;
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::GetTranspose() const
{
  ezSimdMat4f result = *this;
  result.Transpose();
  return result;
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::GetInverse(const ezSimdFloat& fEpsilon) const
{
  ezSimdMat4f result = *this;
  result.Invert(fEpsilon).IgnoreResult();
  return result;
}

inline bool ezSimdMat4f::IsEqual(const ezSimdMat4f& rhs, const ezSimdFloat& fEpsilon) const
{
  return (m_col0.IsEqual(rhs.m_col0, fEpsilon) && m_col1.IsEqual(rhs.m_col1, fEpsilon) && m_col2.IsEqual(rhs.m_col2, fEpsilon) &&
          m_col3.IsEqual(rhs.m_col3, fEpsilon))
    .AllSet<4>();
}

inline bool ezSimdMat4f::IsIdentity(const ezSimdFloat& fEpsilon) const
{
  return (m_col0.IsEqual(ezSimdVec4f(1, 0, 0, 0), fEpsilon) && m_col1.IsEqual(ezSimdVec4f(0, 1, 0, 0), fEpsilon) &&
          m_col2.IsEqual(ezSimdVec4f(0, 0, 1, 0), fEpsilon) && m_col3.IsEqual(ezSimdVec4f(0, 0, 0, 1), fEpsilon))
    .AllSet<4>();
}

inline bool ezSimdMat4f::IsValid() const
{
  return m_col0.IsValid<4>() && m_col1.IsValid<4>() && m_col2.IsValid<4>() && m_col3.IsValid<4>();
}

inline bool ezSimdMat4f::IsNaN() const
{
  return m_col0.IsNaN<4>() || m_col1.IsNaN<4>() || m_col2.IsNaN<4>() || m_col3.IsNaN<4>();
}

EZ_ALWAYS_INLINE void ezSimdMat4f::SetRows(const ezSimdVec4f& vRow0, const ezSimdVec4f& vRow1, const ezSimdVec4f& vRow2, const ezSimdVec4f& vRow3)
{
  m_col0 = vRow0;
  m_col1 = vRow1;
  m_col2 = vRow2;
  m_col3 = vRow3;

  Transpose();
}

EZ_ALWAYS_INLINE void ezSimdMat4f::GetRows(ezSimdVec4f& ref_vRow0, ezSimdVec4f& ref_vRow1, ezSimdVec4f& ref_vRow2, ezSimdVec4f& ref_vRow3) const
{
  ezSimdMat4f tmp = *this;
  tmp.Transpose();

  ref_vRow0 = tmp.m_col0;
  ref_vRow1 = tmp.m_col1;
  ref_vRow2 = tmp.m_col2;
  ref_vRow3 = tmp.m_col3;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdMat4f::TransformPosition(const ezSimdVec4f& v) const
{
  ezSimdVec4f result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();
  result += m_col3;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdMat4f::TransformDirection(const ezSimdVec4f& v) const
{
  ezSimdVec4f result;
  result = m_col0 * v.x();
  result += m_col1 * v.y();
  result += m_col2 * v.z();

  return result;
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::operator*(const ezSimdMat4f& rhs) const
{
  ezSimdMat4f result;

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

EZ_ALWAYS_INLINE void ezSimdMat4f::operator*=(const ezSimdMat4f& rhs)
{
  *this = *this * rhs;
}

EZ_ALWAYS_INLINE bool ezSimdMat4f::operator==(const ezSimdMat4f& other) const
{
  return (m_col0 == other.m_col0 && m_col1 == other.m_col1 && m_col2 == other.m_col2 && m_col3 == other.m_col3).AllSet<4>();
}

EZ_ALWAYS_INLINE bool ezSimdMat4f::operator!=(const ezSimdMat4f& other) const
{
  return !(*this == other);
}

EZ_ALWAYS_INLINE ezSimdMat4f MultiplyAffine(const ezSimdMat4f& lhs, const ezSimdMat4f& rhs)
{
  ezSimdMat4f result;

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

