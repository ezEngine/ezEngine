#pragma once

EZ_ALWAYS_INLINE ezSimdQuat::ezSimdQuat() {}

EZ_ALWAYS_INLINE ezSimdQuat::ezSimdQuat(const ezSimdVec4f& v)
{
  m_v = v;
}

// static
EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::IdentityQuaternion()
{
  return ezSimdQuat(ezSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f));
}

EZ_ALWAYS_INLINE void ezSimdQuat::SetIdentity()
{
  m_v.Set(0.0f, 0.0f, 0.0f, 1.0f);
}

EZ_ALWAYS_INLINE void ezSimdQuat::SetFromAxisAndAngle(const ezSimdVec4f& vRotationAxis, const ezSimdFloat& angle)
{
  ///\todo optimize
  const ezAngle halfAngle = ezAngle::Radian(angle) * 0.5f;
  float s = ezMath::Sin(halfAngle);
  float c = ezMath::Cos(halfAngle);

  m_v = vRotationAxis * s;
  m_v.SetW(c);
}

EZ_ALWAYS_INLINE void ezSimdQuat::Normalize()
{
  m_v.Normalize<4>();
}

inline ezResult ezSimdQuat::GetRotationAxisAndAngle(ezSimdVec4f& vAxis, ezSimdFloat& angle, const ezSimdFloat& fEpsilon) const
{
  ///\todo optimize
  const ezAngle acos = ezMath::ACos(m_v.w());
  const float d = ezMath::Sin(acos);

  if (d < fEpsilon)
  {
    vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    vAxis = m_v / d;
  }

  angle = acos * 2;

  return EZ_SUCCESS;
}

EZ_ALWAYS_INLINE ezSimdMat4f ezSimdQuat::GetAsMat4() const
{
  const ezSimdVec4f xyz = m_v;
  const ezSimdVec4f x2y2z2 = xyz + xyz;
  const ezSimdVec4f xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const ezSimdVec4f yy2_xx2_xx2 = xx2yy2zz2.Get<ezSwizzle::YXXX>();
  const ezSimdVec4f zz2_zz2_yy2 = xx2yy2zz2.Get<ezSwizzle::ZZYX>();
  ezSimdVec4f diagonal = ezSimdVec4f(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(ezSimdFloat::Zero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const ezSimdVec4f x_y_x = xyz.Get<ezSwizzle::XYXX>();
  const ezSimdVec4f y2_z2_z2 = x2y2z2.Get<ezSwizzle::YZZX>();
  const ezSimdVec4f base = x_y_x.CompMul(y2_z2_z2);

  const ezSimdVec4f z2_x2_y2 = x2y2z2.Get<ezSwizzle::ZXYX>();
  const ezSimdVec4f offset = z2_x2_y2 * m_v.w();

  const ezSimdVec4f adds = base + offset;
  const ezSimdVec4f subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const ezSimdVec4f addX_u_diaX_u = adds.GetCombined<ezSwizzle::XXXX>(diagonal);
  const ezSimdVec4f subZ_u_diaW_u = subs.GetCombined<ezSwizzle::ZXWX>(diagonal);
  const ezSimdVec4f col0 = addX_u_diaX_u.GetCombined<ezSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const ezSimdVec4f subX_u_diaY_u = subs.GetCombined<ezSwizzle::XXYX>(diagonal);
  const ezSimdVec4f addY_u_diaW_u = adds.GetCombined<ezSwizzle::YXWX>(diagonal);
  const ezSimdVec4f col1 = subX_u_diaY_u.GetCombined<ezSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const ezSimdVec4f addZ_u_subY_u = adds.GetCombined<ezSwizzle::ZXYX>(subs);
  const ezSimdVec4f col2 = addZ_u_subY_u.GetCombined<ezSwizzle::XZZW>(diagonal);

  return ezSimdMat4f(col0, col1, col2, ezSimdVec4f(0, 0, 0, 1));
}

EZ_ALWAYS_INLINE bool ezSimdQuat::IsValid(const ezSimdFloat& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdQuat::IsNaN() const
{
  return m_v.IsNaN<4>();
}

EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::operator-() const
{
  return m_v.FlipSign(ezSimdVec4b(true, true, true, false));
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdQuat::operator*(const ezSimdVec4f& v) const
{
  ezSimdVec4f t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::operator*(const ezSimdQuat& q2) const
{
  ezSimdQuat q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

EZ_ALWAYS_INLINE bool ezSimdQuat::operator==(const ezSimdQuat& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

EZ_ALWAYS_INLINE bool ezSimdQuat::operator!=(const ezSimdQuat& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}

