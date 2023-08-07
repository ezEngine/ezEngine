#pragma once

EZ_ALWAYS_INLINE ezSimdBBox::ezSimdBBox() = default;

EZ_ALWAYS_INLINE ezSimdBBox::ezSimdBBox(const ezSimdVec4f& vMin, const ezSimdVec4f& vMax)
  : m_Min(vMin)
  , m_Max(vMax)
{
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBox::MakeZero()
{
  return ezSimdBBox(ezSimdVec4f::MakeZero(), ezSimdVec4f::MakeZero());
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBox::MakeInvalid()
{
  return ezSimdBBox(ezSimdVec4f(ezMath::MaxValue<float>()), ezSimdVec4f (- ezMath::MaxValue<float>()));
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBox::MakeFromCenterAndHalfExtents(const ezSimdVec4f& vCenter, const ezSimdVec4f& vHalfExtents)
{
  return ezSimdBBox(vCenter - vHalfExtents, vCenter + vHalfExtents);
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBox::MakeFromMinMax(const ezSimdVec4f& vMin, const ezSimdVec4f& vMax)
{
  return ezSimdBBox(vMin, vMax);
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBox::MakeFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /*= sizeof(ezSimdVec4f)*/)
{
  ezSimdBBox box = ezSimdBBox::MakeInvalid();
  box.ExpandToInclude(pPoints, uiNumPoints, uiStride);
  return box;
}

EZ_ALWAYS_INLINE void ezSimdBBox::SetInvalid()
{
  m_Min.Set(ezMath::MaxValue<float>());
  m_Max.Set(-ezMath::MaxValue<float>());
}

EZ_ALWAYS_INLINE void ezSimdBBox::SetCenterAndHalfExtents(const ezSimdVec4f& vCenter, const ezSimdVec4f& vHalfExtents)
{
  m_Min = vCenter - vHalfExtents;
  m_Max = vCenter + vHalfExtents;
}

EZ_ALWAYS_INLINE void ezSimdBBox::SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  *this = MakeInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

EZ_ALWAYS_INLINE bool ezSimdBBox::IsValid() const
{
  return m_Min.IsValid<3>() && m_Max.IsValid<3>() && (m_Min <= m_Max).AllSet<3>();
}

EZ_ALWAYS_INLINE bool ezSimdBBox::IsNaN() const
{
  return m_Min.IsNaN<3>() || m_Max.IsNaN<3>();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdBBox::GetCenter() const
{
  return (m_Min + m_Max) * ezSimdFloat(0.5f);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdBBox::GetExtents() const
{
  return m_Max - m_Min;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdBBox::GetHalfExtents() const
{
  return (m_Max - m_Min) * ezSimdFloat(0.5f);
}

EZ_ALWAYS_INLINE void ezSimdBBox::ExpandToInclude(const ezSimdVec4f& vPoint)
{
  m_Min = m_Min.CompMin(vPoint);
  m_Max = m_Max.CompMax(vPoint);
}

inline void ezSimdBBox::ExpandToInclude(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezSimdVec4f), "Data may not overlap.");

  const ezSimdVec4f* pCur = pPoints;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = ezMemoryUtils::AddByteOffset(pCur, uiStride);
  }
}

EZ_ALWAYS_INLINE void ezSimdBBox::ExpandToInclude(const ezSimdBBox& rhs)
{
  m_Min = m_Min.CompMin(rhs.m_Min);
  m_Max = m_Max.CompMax(rhs.m_Max);
}

inline void ezSimdBBox::ExpandToCube()
{
  const ezSimdVec4f center = GetCenter();
  const ezSimdVec4f halfExtents = center - m_Min;

  *this = ezSimdBBox::MakeFromCenterAndHalfExtents(center, ezSimdVec4f(halfExtents.HorizontalMax<3>()));
}

EZ_ALWAYS_INLINE bool ezSimdBBox::Contains(const ezSimdVec4f& vPoint) const
{
  return ((vPoint >= m_Min) && (vPoint <= m_Max)).AllSet<3>();
}

EZ_ALWAYS_INLINE bool ezSimdBBox::Contains(const ezSimdBBox& rhs) const
{
  return Contains(rhs.m_Min) && Contains(rhs.m_Max);
}

inline bool ezSimdBBox::Contains(const ezSimdBSphere& rhs) const
{
  const ezSimdBBox otherBox = ezSimdBBox::MakeFromCenterAndHalfExtents(rhs.GetCenter(), ezSimdVec4f(rhs.GetRadius()));

  return Contains(otherBox);
}

EZ_ALWAYS_INLINE bool ezSimdBBox::Overlaps(const ezSimdBBox& rhs) const
{
  return ((m_Max > rhs.m_Min) && (m_Min < rhs.m_Max)).AllSet<3>();
}

inline bool ezSimdBBox::Overlaps(const ezSimdBSphere& rhs) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return rhs.Contains(GetClampedPoint(rhs.GetCenter()));
}

EZ_ALWAYS_INLINE void ezSimdBBox::Grow(const ezSimdVec4f& vDiff)
{
  m_Max += vDiff;
  m_Min -= vDiff;
}

EZ_ALWAYS_INLINE void ezSimdBBox::Translate(const ezSimdVec4f& vDiff)
{
  m_Min += vDiff;
  m_Max += vDiff;
}

EZ_ALWAYS_INLINE void ezSimdBBox::Transform(const ezSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

EZ_ALWAYS_INLINE void ezSimdBBox::Transform(const ezSimdMat4f& mMat)
{
  const ezSimdVec4f center = GetCenter();
  const ezSimdVec4f halfExtents = center - m_Min;

  const ezSimdVec4f newCenter = mMat.TransformPosition(center);

  ezSimdVec4f newHalfExtents = mMat.m_col0.Abs() * halfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * halfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * halfExtents.z();

  *this = ezSimdBBox::MakeFromCenterAndHalfExtents(newCenter, newHalfExtents);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdBBox::GetClampedPoint(const ezSimdVec4f& vPoint) const
{
  return vPoint.CompMin(m_Max).CompMax(m_Min);
}

inline ezSimdFloat ezSimdBBox::GetDistanceSquaredTo(const ezSimdVec4f& vPoint) const
{
  const ezSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared<3>();
}

inline ezSimdFloat ezSimdBBox::GetDistanceTo(const ezSimdVec4f& vPoint) const
{
  const ezSimdVec4f vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength<3>();
}

EZ_ALWAYS_INLINE bool ezSimdBBox::operator==(const ezSimdBBox& rhs) const
{
  return ((m_Min == rhs.m_Min) && (m_Max == rhs.m_Max)).AllSet<3>();
}

EZ_ALWAYS_INLINE bool ezSimdBBox::operator!=(const ezSimdBBox& rhs) const
{
  return ((m_Min != rhs.m_Min) || (m_Max != rhs.m_Max)).AnySet<3>();
}
