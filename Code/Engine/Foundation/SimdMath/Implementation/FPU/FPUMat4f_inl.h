#pragma once

EZ_ALWAYS_INLINE void ezSimdMat4f::Transpose()
{
  ezMath::Swap(m_col0.m_v.y, m_col1.m_v.x);
  ezMath::Swap(m_col0.m_v.z, m_col2.m_v.x);
  ezMath::Swap(m_col0.m_v.w, m_col3.m_v.x);
  ezMath::Swap(m_col1.m_v.z, m_col2.m_v.y);
  ezMath::Swap(m_col1.m_v.w, m_col3.m_v.y);
  ezMath::Swap(m_col2.m_v.w, m_col3.m_v.z);
}
