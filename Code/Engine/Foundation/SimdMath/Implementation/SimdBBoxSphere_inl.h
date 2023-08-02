#pragma once

EZ_ALWAYS_INLINE ezSimdBBoxSphere::ezSimdBBoxSphere() = default;

EZ_ALWAYS_INLINE ezSimdBBoxSphere::ezSimdBBoxSphere(const ezSimdVec4f& vCenter, const ezSimdVec4f& vBoxHalfExtents, const ezSimdFloat& fSphereRadius)
  : m_CenterAndRadius(vCenter)
  , m_BoxHalfExtents(vBoxHalfExtents)
{
  m_CenterAndRadius.SetW(fSphereRadius);
}

inline ezSimdBBoxSphere::ezSimdBBoxSphere(const ezSimdBBox& box, const ezSimdBSphere& sphere)
  : m_CenterAndRadius(box.GetCenter())
  , m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>().Min((sphere.GetCenter() - m_CenterAndRadius).GetLength<3>() + sphere.GetRadius()));
}

inline ezSimdBBoxSphere::ezSimdBBoxSphere(const ezSimdBBox& box)
  : m_CenterAndRadius(box.GetCenter())
  , m_BoxHalfExtents(m_CenterAndRadius - box.m_Min)
{
  m_CenterAndRadius.SetW(m_BoxHalfExtents.GetLength<3>());
}

EZ_ALWAYS_INLINE ezSimdBBoxSphere::ezSimdBBoxSphere(const ezSimdBSphere& sphere)
  : m_CenterAndRadius(sphere.m_CenterAndRadius)
  , m_BoxHalfExtents(ezSimdVec4f(sphere.GetRadius()))
{
}

EZ_ALWAYS_INLINE void ezSimdBBoxSphere::SetInvalid()
{
  m_CenterAndRadius.Set(0.0f, 0.0f, 0.0f, -ezMath::SmallEpsilon<float>());
  m_BoxHalfExtents.Set(-ezMath::MaxValue<float>());
}

EZ_ALWAYS_INLINE bool ezSimdBBoxSphere::IsValid() const
{
  return m_CenterAndRadius.IsValid<4>() && m_CenterAndRadius.w() >= ezSimdFloat::MakeZero() && m_BoxHalfExtents.IsValid<3>() &&
         (m_BoxHalfExtents >= ezSimdVec4f::MakeZero()).AllSet<3>();
}

inline bool ezSimdBBoxSphere::IsNaN() const
{
  return m_CenterAndRadius.IsNaN<4>() || m_BoxHalfExtents.IsNaN<3>();
}

inline void ezSimdBBoxSphere::SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  ezSimdBBox box;
  box.SetFromPoints(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius = box.GetCenter();
  m_BoxHalfExtents = m_CenterAndRadius - box.m_Min;

  ezSimdBSphere sphere(m_CenterAndRadius, ezSimdFloat::MakeZero());
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  m_CenterAndRadius.SetW(sphere.GetRadius());
}

EZ_ALWAYS_INLINE ezSimdBBox ezSimdBBoxSphere::GetBox() const
{
  ezSimdBBox box;
  box.SetCenterAndHalfExtents(m_CenterAndRadius, m_BoxHalfExtents);
  return box;
}

EZ_ALWAYS_INLINE ezSimdBSphere ezSimdBBoxSphere::GetSphere() const
{
  ezSimdBSphere sphere;
  sphere.m_CenterAndRadius = m_CenterAndRadius;
  return sphere;
}

inline void ezSimdBBoxSphere::ExpandToInclude(const ezSimdBBoxSphere& rhs)
{
  ezSimdBBox box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  ezSimdVec4f center = box.GetCenter();
  ezSimdVec4f boxHalfExtents = center - box.m_Min;
  ezSimdFloat tmpRadius = boxHalfExtents.GetLength<3>();

  const ezSimdFloat fSphereRadiusA = (m_CenterAndRadius - center).GetLength<3>() + m_CenterAndRadius.w();
  const ezSimdFloat fSphereRadiusB = (rhs.m_CenterAndRadius - center).GetLength<3>() + rhs.m_CenterAndRadius.w();

  m_CenterAndRadius = center;
  m_CenterAndRadius.SetW(tmpRadius.Min(fSphereRadiusA.Max(fSphereRadiusB)));
  m_BoxHalfExtents = boxHalfExtents;
}

EZ_ALWAYS_INLINE void ezSimdBBoxSphere::Transform(const ezSimdTransform& t)
{
  Transform(t.GetAsMat4());
}

EZ_ALWAYS_INLINE void ezSimdBBoxSphere::Transform(const ezSimdMat4f& mMat)
{
  ezSimdFloat radius = m_CenterAndRadius.w();
  m_CenterAndRadius = mMat.TransformPosition(m_CenterAndRadius);

  ezSimdFloat maxRadius = mMat.m_col0.Dot<3>(mMat.m_col0);
  maxRadius = maxRadius.Max(mMat.m_col1.Dot<3>(mMat.m_col1));
  maxRadius = maxRadius.Max(mMat.m_col2.Dot<3>(mMat.m_col2));
  radius *= maxRadius.GetSqrt();

  m_CenterAndRadius.SetW(radius);

  ezSimdVec4f newHalfExtents = mMat.m_col0.Abs() * m_BoxHalfExtents.x();
  newHalfExtents += mMat.m_col1.Abs() * m_BoxHalfExtents.y();
  newHalfExtents += mMat.m_col2.Abs() * m_BoxHalfExtents.z();

  m_BoxHalfExtents = newHalfExtents.CompMin(ezSimdVec4f(radius));
}

EZ_ALWAYS_INLINE bool ezSimdBBoxSphere::operator==(const ezSimdBBoxSphere& rhs) const
{
  return (m_CenterAndRadius == rhs.m_CenterAndRadius).AllSet<4>() && (m_BoxHalfExtents == rhs.m_BoxHalfExtents).AllSet<3>();
}

EZ_ALWAYS_INLINE bool ezSimdBBoxSphere::operator!=(const ezSimdBBoxSphere& rhs) const
{
  return !(*this == rhs);
}
