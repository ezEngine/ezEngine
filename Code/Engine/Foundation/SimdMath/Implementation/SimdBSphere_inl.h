#pragma once

EZ_ALWAYS_INLINE ezSimdBSphere::ezSimdBSphere()
{

}

EZ_ALWAYS_INLINE ezSimdBSphere::ezSimdBSphere(const ezSimdVec4f& vCenter, const ezSimdFloat& fRadius)
{
  m_CenterAndRadius = vCenter;
  m_CenterAndRadius.SetW(fRadius);
}

EZ_ALWAYS_INLINE void ezSimdBSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -1.0f);
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= ezSimdFloat::Zero();
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdBSphere::GetCenter() const
{
  return m_CenterAndRadius;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdBSphere::GetRadius() const
{
  return m_CenterAndRadius.w();
}

inline void ezSimdBSphere::ExpandToInclude(const ezSimdVec4f& vPoint)
{
  const ezSimdFloat fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void ezSimdBSphere::ExpandToInclude(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezSimdVec4f), "The data must not overlap.");

  const ezSimdVec4f* pCur = pPoints;

  ezSimdFloat fMaxDistSquare = ezSimdFloat::Zero();

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const ezSimdFloat fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

inline void ezSimdBSphere::ExpandToInclude(const ezSimdBSphere& rhs)
{
  const ezSimdFloat fReqRadius = (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius();

  m_CenterAndRadius.SetW(fReqRadius.Max(GetRadius()));
}

inline void ezSimdBSphere::Transform(const ezSimdTransform& t)
{
  ezSimdVec4f newCenterAndRadius = t.TransformPosition(m_CenterAndRadius);
  newCenterAndRadius.SetW(t.GetMaxScale() * GetRadius());

  m_CenterAndRadius = newCenterAndRadius;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdBSphere::GetDistanceTo(const ezSimdVec4f& vPoint) const
{
  return (vPoint - m_CenterAndRadius).GetLength<3>() - GetRadius();
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdBSphere::GetDistanceTo(const ezSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() - GetRadius() - rhs.GetRadius();
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::Contains(const ezSimdVec4f& vPoint) const
{
  ezSimdFloat radius = GetRadius();
  return (vPoint - m_CenterAndRadius).GetLengthSquared<3>() <= (radius * radius);
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::Contains(const ezSimdBSphere& rhs) const
{
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLength<3>() + rhs.GetRadius() <= GetRadius();
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::Overlaps(const ezSimdBSphere& rhs) const
{
  ezSimdFloat radius = (rhs.m_CenterAndRadius + m_CenterAndRadius).w();
  return (rhs.m_CenterAndRadius - m_CenterAndRadius).GetLengthSquared<3>() < (radius * radius);
}

inline ezSimdVec4f ezSimdBSphere::GetClampedPoint(const ezSimdVec4f& vPoint)
{
  ezSimdVec4f vDir = vPoint - m_CenterAndRadius;
  ezSimdFloat fDist = vDir.GetLengthAndNormalize<3>().Min(GetRadius());

  return m_CenterAndRadius + (vDir * fDist);
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::operator==(const ezSimdBSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet();
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::operator!=(const ezSimdBSphere& rhs) const
{
  return (m_CenterAndRadius != rhs.m_CenterAndRadius).AnySet();
}
