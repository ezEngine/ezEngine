#pragma once

EZ_ALWAYS_INLINE void ezSimdMat4d::Transpose()
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  #error
#else
  _MM_TRANSPOSE4_PD(
    m_col0.m_v.xy, m_col0.m_v.zw, 
    m_col1.m_v.xy, m_col1.m_v.zw, 
    m_col2.m_v.xy, m_col2.m_v.zw, 
    m_col3.m_v.xy, m_col3.m_v.zw);
#endif

}
