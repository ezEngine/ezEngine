#pragma once

template <typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vOrigin and m_vBoxHalfExtends are already initialized to NaN by their own constructor.
  const Type TypeNaN = ezMath::NaN<Type>();
  m_fSphereRadius = TypeNaN;
#endif
}

template <typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtends = rhs.m_vBoxHalfExtends;
}

template <typename Type>
void ezBoundingBoxSphereTemplate<Type>::operator=(const ezBoundingBoxSphereTemplate& rhs)
{
  m_vCenter = rhs.m_vCenter;
  m_fSphereRadius = rhs.m_fSphereRadius;
  m_vBoxHalfExtends = rhs.m_vBoxHalfExtends;
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingBoxTemplate<Type>& box)
  : m_vCenter(box.GetCenter())
{
  m_vBoxHalfExtends = box.GetHalfExtents();
  m_fSphereRadius = m_vBoxHalfExtends.GetLength();
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingSphereTemplate<Type>& sphere)
  : m_vCenter(sphere.m_vCenter)
  , m_fSphereRadius(sphere.m_fRadius)
{
  m_vBoxHalfExtends.Set(m_fSphereRadius);
}


template <typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeZero()
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = 0;
  res.m_vBoxHalfExtends.SetZero();
  return res;
}

template <typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeInvalid()
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter.SetZero();
  res.m_fSphereRadius = -ezMath::SmallEpsilon<Type>(); // has to be very small for ExpandToInclude to work
  res.m_vBoxHalfExtends.Set(-ezMath::MaxValue<Type>());
  return res;
}

template <typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeFromCenterExtents(const ezVec3Template<Type>& vCenter, const ezVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius)
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = vCenter;
  res.m_fSphereRadius = fSphereRadius;
  res.m_vBoxHalfExtends = vBoxHalfExtents;
  return res;
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeFromPoints(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /*= sizeof(ezVec3Template<Type>)*/)
{
  ezBoundingBoxTemplate<Type> box = ezBoundingBoxTemplate<Type>::MakeFromPoints(pPoints, uiNumPoints, uiStride);

  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();

  ezBoundingSphereTemplate<Type> sphere = ezBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(res.m_vCenter, 0.0f);
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  res.m_fSphereRadius = sphere.m_fRadius;
  return res;
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeFromBox(const ezBoundingBoxTemplate<Type>& box)
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();
  res.m_fSphereRadius = res.m_vBoxHalfExtends.GetLength();
  return res;
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeFromSphere(const ezBoundingSphereTemplate<Type>& sphere)
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = sphere.m_vCenter;
  res.m_fSphereRadius = sphere.m_fRadius;
  res.m_vBoxHalfExtends.Set(res.m_fSphereRadius);
  return res;
}

template <typename Type>
ezBoundingBoxSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::MakeFromBoxAndSphere(const ezBoundingBoxTemplate<Type>& box, const ezBoundingSphereTemplate<Type>& sphere)
{
  ezBoundingBoxSphereTemplate<Type> res;
  res.m_vCenter = box.GetCenter();
  res.m_vBoxHalfExtends = box.GetHalfExtents();
  res.m_fSphereRadius = ezMath::Min(res.m_vBoxHalfExtends.GetLength(), (sphere.m_vCenter - res.m_vCenter).GetLength() + sphere.m_fRadius);
  return res;
}

template <typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fSphereRadius >= 0.0f && m_vBoxHalfExtends.IsValid() && (m_vBoxHalfExtends.x >= 0) && (m_vBoxHalfExtends.y >= 0) && (m_vBoxHalfExtends.z >= 0));
}

template <typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || ezMath::IsNaN(m_fSphereRadius) || m_vBoxHalfExtends.IsNaN());
}

template <typename Type>
EZ_FORCE_INLINE const ezBoundingBoxTemplate<Type> ezBoundingBoxSphereTemplate<Type>::GetBox() const
{
  return ezBoundingBoxTemplate<Type>::MakeFromMinMax(m_vCenter - m_vBoxHalfExtends, m_vCenter + m_vBoxHalfExtends);
}

template <typename Type>
EZ_FORCE_INLINE const ezBoundingSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::GetSphere() const
{
  return ezBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(m_vCenter, m_fSphereRadius);
}

template <typename Type>
void ezBoundingBoxSphereTemplate<Type>::ExpandToInclude(const ezBoundingBoxSphereTemplate& rhs)
{
  ezBoundingBoxTemplate<Type> box;
  box.m_vMin = m_vCenter - m_vBoxHalfExtends;
  box.m_vMax = m_vCenter + m_vBoxHalfExtends;
  box.ExpandToInclude(rhs.GetBox());

  ezBoundingBoxSphereTemplate<Type> result = ezBoundingBoxSphereTemplate<Type>::MakeFromBox(box);

  const float fSphereRadiusA = (m_vCenter - result.m_vCenter).GetLength() + m_fSphereRadius;
  const float fSphereRadiusB = (rhs.m_vCenter - result.m_vCenter).GetLength() + rhs.m_fSphereRadius;

  m_vCenter = result.m_vCenter;
  m_fSphereRadius = ezMath::Min(result.m_fSphereRadius, ezMath::Max(fSphereRadiusA, fSphereRadiusB));
  m_vBoxHalfExtends = result.m_vBoxHalfExtends;
}

template <typename Type>
void ezBoundingBoxSphereTemplate<Type>::Transform(const ezMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition(m_vCenter);
  const ezVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fSphereRadius *= ezMath::Max(Scale.x, Scale.y, Scale.z);

  ezMat3Template<Type> mAbsRotation = mTransform.GetRotationalPart();
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    mAbsRotation.m_fElementsCM[i] = ezMath::Abs(mAbsRotation.m_fElementsCM[i]);
  }

  m_vBoxHalfExtends = mAbsRotation.TransformDirection(m_vBoxHalfExtends).CompMin(ezVec3(m_fSphereRadius));
}

template <typename Type>
EZ_FORCE_INLINE bool operator==(const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs)
{
  return lhs.m_vCenter == rhs.m_vCenter && lhs.m_vBoxHalfExtends == rhs.m_vBoxHalfExtends && lhs.m_fSphereRadius == rhs.m_fSphereRadius;
}

/// \brief Checks whether this box and the other are not identical.
template <typename Type>
EZ_ALWAYS_INLINE bool operator!=(const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs)
{
  return !(lhs == rhs);
}
