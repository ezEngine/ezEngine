#pragma once

EZ_ALWAYS_INLINE void ezSimdMat4f::Transpose()
{
  float32x4x2_t P0 = vzipq_f32(m_col0.m_v, m_col2.m_v);
  float32x4x2_t P1 = vzipq_f32(m_col1.m_v, m_col3.m_v);

  float32x4x2_t T0 = vzipq_f32(P0.val[0], P1.val[0]);
  float32x4x2_t T1 = vzipq_f32(P0.val[1], P1.val[1]);

  m_col0.m_v = T0.val[0];
  m_col1.m_v = T0.val[1];
  m_col2.m_v = T1.val[0];
  m_col3.m_v = T1.val[1];
}
