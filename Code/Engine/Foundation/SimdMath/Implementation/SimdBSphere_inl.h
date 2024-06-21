#pragma once

EZ_ALWAYS_INLINE ezSimdBSphere::ezSimdBSphere() = default;

EZ_ALWAYS_INLINE ezSimdBSphere::ezSimdBSphere(const ezSimdVec4f& vCenter, const ezSimdFloat& fRadius)
  : m_CenterAndRadius(vCenter)
{
  m_CenterAndRadius.SetW(fRadius);
}

EZ_ALWAYS_INLINE ezSimdBSphere ezSimdBSphere::MakeZero()
{
  ezSimdBSphere res;
  res.m_CenterAndRadius = ezSimdVec4f::MakeZero();
  return res;
}

EZ_ALWAYS_INLINE ezSimdBSphere ezSimdBSphere::MakeInvalid(const ezSimdVec4f& vCenter /*= ezSimdVec4f::MakeZero()*/)
{
  ezSimdBSphere res;
  res.m_CenterAndRadius = vCenter;
  res.m_CenterAndRadius.SetW(-ezMath::SmallEpsilon<float>());
  return res;
}

EZ_ALWAYS_INLINE ezSimdBSphere ezSimdBSphere::MakeFromCenterAndRadius(const ezSimdVec4f& vCenter, const ezSimdFloat& fRadius)
{
  return ezSimdBSphere(vCenter, fRadius);
}

inline ezSimdBSphere ezSimdBSphere::MakeFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /*= sizeof(ezSimdVec4f)*/)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezSimdVec4f), "The data must not overlap.");
  EZ_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  ezSimdBSphere res;

  const ezSimdVec4f* pCur = pPoints;

  ezSimdVec4f vCenter = ezSimdVec4f::MakeZero();
  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius = vCenter / ezSimdFloat(uiNumPoints);

  pCur = pPoints;

  ezSimdFloat fMaxDistSquare = ezSimdFloat::MakeZero();
  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const ezSimdFloat fDistSQR = (*pCur - res.m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  res.m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt());

  return res;
}

EZ_ALWAYS_INLINE void ezSimdBSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -ezMath::SmallEpsilon<float>());
}

EZ_ALWAYS_INLINE bool ezSimdBSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && GetRadius() >= ezSimdFloat::MakeZero();
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

EZ_ALWAYS_INLINE void ezSimdBSphere::SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  *this = MakeFromPoints(pPoints, uiNumPoints, uiStride);
}

EZ_ALWAYS_INLINE void ezSimdBSphere::ExpandToInclude(const ezSimdVec4f& vPoint)
{
  const ezSimdFloat fDist = (vPoint - m_CenterAndRadius).GetLength<3>();

  m_CenterAndRadius.SetW(fDist.Max(GetRadius()));
}

inline void ezSimdBSphere::ExpandToInclude(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezSimdVec4f), "The data must not overlap.");

  const ezSimdVec4f* pCur = pPoints;

  ezSimdFloat fMaxDistSquare = ezSimdFloat::MakeZero();

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const ezSimdFloat fDistSQR = (*pCur - m_CenterAndRadius).GetLengthSquared<3>();
    fMaxDistSquare = fMaxDistSquare.Max(fDistSQR);

    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }

  m_CenterAndRadius.SetW(fMaxDistSquare.GetSqrt().Max(GetRadius()));
}

EZ_ALWAYS_INLINE void ezSimdBSphere::ExpandToInclude(const ezSimdBSphere& rhs)
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

inline void ezSimdBSphere::Transform(const ezSimdMat4f& mMat)
{
  ezSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  ezSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);
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
