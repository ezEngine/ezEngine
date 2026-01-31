#pragma once

EZ_ALWAYS_INLINE ezSimdQuatd::ezSimdQuatd() = default;

EZ_ALWAYS_INLINE ezSimdQuatd::ezSimdQuatd(const ezSimdVec4d& v)
  : m_v(v)
{
}

EZ_ALWAYS_INLINE const ezSimdQuatd ezSimdQuatd::MakeIdentity()
{
  return ezSimdQuatd(ezSimdVec4d(0.0f, 0.0f, 0.0f, 1.0f));
}

EZ_ALWAYS_INLINE ezSimdQuatd ezSimdQuatd::MakeFromElements(ezSimdDouble x, ezSimdDouble y, ezSimdDouble z, ezSimdDouble w)
{
  return ezSimdQuatd(ezSimdVec4d(x, y, z, w));
}

inline ezSimdQuatd ezSimdQuatd::MakeFromAxisAndAngle(const ezSimdVec4d& vRotationAxis, const ezSimdDouble& fAngle)
{
  ///\todo optimize
  const ezAngled halfAngle = ezAngled::MakeFromRadian(fAngle) * 0.5;
  double s = ezMath::Sin(halfAngle);
  double c = ezMath::Cos(halfAngle);

  ezSimdQuatd res;
  res.m_v = vRotationAxis * s;
  res.m_v.SetW(c);
  return res;
}

EZ_ALWAYS_INLINE void ezSimdQuatd::Normalize()
{
  m_v.Normalize<4>();
}

inline ezResult ezSimdQuatd::GetRotationAxisAndAngle(ezSimdVec4d& ref_vAxis, ezSimdDouble& ref_fAngle, const ezSimdDouble& fEpsilon) const
{
  ///\todo optimize
  const ezAngleTemplate<double> acos = ezMath::ACos<double>(m_v.w().Max(-1).Min(1));
  const float d = ezMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = (acos * double(2)).GetRadian();

  return EZ_SUCCESS;
}

EZ_ALWAYS_INLINE ezSimdMat4d ezSimdQuatd::GetAsMat4() const
{
  const ezSimdVec4d xyz = m_v;
  const ezSimdVec4d x2y2z2 = xyz + xyz;
  const ezSimdVec4d xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const ezSimdVec4d yy2_xx2_xx2 = xx2yy2zz2.Get<ezSwizzle::YXXX>();
  const ezSimdVec4d zz2_zz2_yy2 = xx2yy2zz2.Get<ezSwizzle::ZZYX>();
  ezSimdVec4d diagonal = ezSimdVec4d(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(ezSimdDouble::MakeZero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const ezSimdVec4d x_y_x = xyz.Get<ezSwizzle::XYXX>();
  const ezSimdVec4d y2_z2_z2 = x2y2z2.Get<ezSwizzle::YZZX>();
  const ezSimdVec4d base = x_y_x.CompMul(y2_z2_z2);

  const ezSimdVec4d z2_x2_y2 = x2y2z2.Get<ezSwizzle::ZXYX>();
  const ezSimdVec4d offset = z2_x2_y2 * m_v.w();

  const ezSimdVec4d adds = base + offset;
  const ezSimdVec4d subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const ezSimdVec4d addX_u_diaX_u = adds.GetCombined<ezSwizzle::XXXX>(diagonal);
  const ezSimdVec4d subZ_u_diaW_u = subs.GetCombined<ezSwizzle::ZXWX>(diagonal);
  const ezSimdVec4d col0 = addX_u_diaX_u.GetCombined<ezSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const ezSimdVec4d subX_u_diaY_u = subs.GetCombined<ezSwizzle::XXYX>(diagonal);
  const ezSimdVec4d addY_u_diaW_u = adds.GetCombined<ezSwizzle::YXWX>(diagonal);
  const ezSimdVec4d col1 = subX_u_diaY_u.GetCombined<ezSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const ezSimdVec4d addZ_u_subY_u = adds.GetCombined<ezSwizzle::ZXYX>(subs);
  const ezSimdVec4d col2 = addZ_u_subY_u.GetCombined<ezSwizzle::XZZW>(diagonal);

  return ezSimdMat4d::MakeFromColumns(col0, col1, col2, ezSimdVec4d(0.0, 0.0, 0.0, 1.0));
}

EZ_ALWAYS_INLINE bool ezSimdQuatd::IsValid(const ezSimdDouble& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdQuatd::IsNaN() const
{
  return m_v.IsNaN<4>();
}

EZ_ALWAYS_INLINE ezSimdQuatd ezSimdQuatd::operator-() const
{
  return ezSimdQuatd(m_v.FlipSign(ezSimdVec4bWide(true, true, true, false)));
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdQuatd::operator*(const ezSimdVec4d& v) const
{
  ezSimdVec4d t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

EZ_ALWAYS_INLINE ezSimdQuatd ezSimdQuatd::operator*(const ezSimdQuatd& q2) const
{
  ezSimdQuatd q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

EZ_ALWAYS_INLINE bool ezSimdQuatd::operator==(const ezSimdQuatd& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

EZ_ALWAYS_INLINE bool ezSimdQuatd::operator!=(const ezSimdQuatd& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
