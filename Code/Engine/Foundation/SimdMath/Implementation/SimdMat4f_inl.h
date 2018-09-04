#pragma once

EZ_ALWAYS_INLINE ezSimdMat4f::ezSimdMat4f() {}

EZ_ALWAYS_INLINE ezSimdMat4f::ezSimdMat4f(const float* const pData, ezMatrixLayout::Enum layout)
{
  SetFromArray(pData, layout);
}

EZ_ALWAYS_INLINE ezSimdMat4f::ezSimdMat4f(const ezSimdVec4f& col0, const ezSimdVec4f& col1, const ezSimdVec4f& col2,
                                          const ezSimdVec4f& col3)
{
  m_col0 = col0;
  m_col1 = col1;
  m_col2 = col2;
  m_col3 = col3;
}

EZ_ALWAYS_INLINE ezSimdMat4f::ezSimdMat4f(float c1r1, float c2r1, float c3r1, float c4r1, float c1r2, float c2r2, float c3r2, float c4r2,
                                          float c1r3, float c2r3, float c3r3, float c4r3, float c1r4, float c2r4, float c3r4, float c4r4)
{
  m_col0.Set(c1r1, c1r2, c1r3, c1r4);
  m_col1.Set(c2r1, c2r2, c2r3, c2r4);
  m_col2.Set(c3r1, c3r2, c3r3, c3r4);
  m_col3.Set(c4r1, c4r2, c4r3, c4r4);
}

inline void ezSimdMat4f::SetFromArray(const float* const pData, ezMatrixLayout::Enum layout)
{
  m_col0.Load<4>(pData + 0);
  m_col1.Load<4>(pData + 4);
  m_col2.Load<4>(pData + 8);
  m_col3.Load<4>(pData + 12);

  if (layout == ezMatrixLayout::RowMajor)
  {
    Transpose();
  }
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

EZ_ALWAYS_INLINE void ezSimdMat4f::SetIdentity()
{
  m_col0.Set(1, 0, 0, 0);
  m_col1.Set(0, 1, 0, 0);
  m_col2.Set(0, 0, 1, 0);
  m_col3.Set(0, 0, 0, 1);
}

// static
EZ_ALWAYS_INLINE ezSimdMat4f ezSimdMat4f::IdentityMatrix()
{
  ezSimdMat4f result;
  result.SetIdentity();
  return result;
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
  result.Invert(fEpsilon);
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

EZ_ALWAYS_INLINE void ezSimdMat4f::SetRows(const ezSimdVec4f& row0, const ezSimdVec4f& row1, const ezSimdVec4f& row2,
                                           const ezSimdVec4f& row3)
{
  m_col0 = row0;
  m_col1 = row1;
  m_col2 = row2;
  m_col3 = row3;

  Transpose();
}

EZ_ALWAYS_INLINE void ezSimdMat4f::GetRows(ezSimdVec4f& row0, ezSimdVec4f& row1, ezSimdVec4f& row2, ezSimdVec4f& row3) const
{
  ezSimdMat4f tmp = *this;
  tmp.Transpose();

  row0 = tmp.m_col0;
  row1 = tmp.m_col1;
  row2 = tmp.m_col2;
  row3 = tmp.m_col3;
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
