#pragma once

EZ_ALWAYS_INLINE void ezSimdMat4d::Transpose()
{
  // Transpose a 4x4 double matrix stored in four float64x2_t pairs
  // Input: col0 = (x0, y0, z0, w0), col1 = (x1, y1, z1, w1), etc.
  // Output: col0 = (x0, x1, x2, x3), col1 = (y0, y1, y2, y3), etc.

  // Extract all xy and zw halves
  float64x2_t c0xy = m_col0.m_v.xy;
  float64x2_t c0zw = m_col0.m_v.zw;
  float64x2_t c1xy = m_col1.m_v.xy;
  float64x2_t c1zw = m_col1.m_v.zw;
  float64x2_t c2xy = m_col2.m_v.xy;
  float64x2_t c2zw = m_col2.m_v.zw;
  float64x2_t c3xy = m_col3.m_v.xy;
  float64x2_t c3zw = m_col3.m_v.zw;

  // Transpose 2x2 blocks
  // [x0 y0]   [x1 y1]     [x0 x1]   [y0 y1]
  // [z0 w0]   [z1 w1] --> [z0 z1]   [w0 w1]
  // [x2 y2]   [x3 y3]     [x2 x3]   [y2 y3]
  // [z2 w2]   [z3 w3]     [z2 z3]   [w2 w3]

  // vtrn/vzip for 2-element pairs
  float64x2_t x0x1 = vzip1q_f64(c0xy, c1xy); // [x0, x1]
  float64x2_t y0y1 = vzip2q_f64(c0xy, c1xy); // [y0, y1]
  float64x2_t z0z1 = vzip1q_f64(c0zw, c1zw); // [z0, z1]
  float64x2_t w0w1 = vzip2q_f64(c0zw, c1zw); // [w0, w1]

  float64x2_t x2x3 = vzip1q_f64(c2xy, c3xy); // [x2, x3]
  float64x2_t y2y3 = vzip2q_f64(c2xy, c3xy); // [y2, y3]
  float64x2_t z2z3 = vzip1q_f64(c2zw, c3zw); // [z2, z3]
  float64x2_t w2w3 = vzip2q_f64(c2zw, c3zw); // [w2, w3]

  // Assign result
  m_col0.m_v.xy = x0x1;
  m_col0.m_v.zw = x2x3;
  m_col1.m_v.xy = y0y1;
  m_col1.m_v.zw = y2y3;
  m_col2.m_v.xy = z0z1;
  m_col2.m_v.zw = z2z3;
  m_col3.m_v.xy = w0w1;
  m_col3.m_v.zw = w2w3;
}