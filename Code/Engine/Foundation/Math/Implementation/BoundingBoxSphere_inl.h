#pragma once

template<typename Type>
EZ_FORCE_INLINE ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vOrigin and m_vBoxHalfExtends are already initialized to NaN by their own constructor.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  m_fSphereRadius = TypeNaN;
#endif
}

template<typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezVec3Template<Type>& vCenter, const ezVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius)
{
  m_vCenter = vCenter;
  m_fSphereRadius = fSphereRadius;
  m_vBoxHalfExtends = vBoxHalfExtents;
}

template<typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingBoxTemplate<Type>& box, const ezBoundingSphereTemplate<Type>& sphere)
{
  m_vCenter = box.GetCenter();
  m_vBoxHalfExtends = box.GetHalfExtents();
  m_fSphereRadius = ezMath::Min(m_vBoxHalfExtends.GetLength(), (sphere.m_vCenter - m_vCenter).GetLength() + sphere.m_fRadius);
}

template<typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingBoxTemplate<Type>& box)
{
  m_vCenter = box.GetCenter();
  m_vBoxHalfExtends = box.GetHalfExtents();
  m_fSphereRadius = m_vBoxHalfExtends.GetLength();
}

template<typename Type>
ezBoundingBoxSphereTemplate<Type>::ezBoundingBoxSphereTemplate(const ezBoundingSphereTemplate<Type>& sphere)
{
  m_vCenter = sphere.m_vCenter;
  m_fSphereRadius = sphere.m_fRadius;
  m_vBoxHalfExtends.Set(m_fSphereRadius);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxSphereTemplate<Type>::SetInvalid()
{
  m_vCenter.SetZero();
  m_fSphereRadius = -1.0f;
  m_vBoxHalfExtends.SetZero();
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fSphereRadius >= 0.0f && m_vBoxHalfExtends.IsValid());
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || ezMath::IsNaN(m_fSphereRadius) || m_vBoxHalfExtends.IsNaN());
}

template<typename Type>
void ezBoundingBoxSphereTemplate<Type>::SetFromPoints(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  ezBoundingBoxTemplate<Type> box;
  box.SetFromPoints(pPoints, uiNumPoints, uiStride);

  m_vCenter = box.GetCenter();
  m_vBoxHalfExtends = box.GetHalfExtents();

  ezBoundingSphereTemplate<Type> sphere(m_vCenter, 0.0f);
  sphere.ExpandToInclude(pPoints, uiNumPoints, uiStride);

  m_fSphereRadius = sphere.m_fRadius;
}

template<typename Type>
EZ_FORCE_INLINE const ezBoundingBoxTemplate<Type> ezBoundingBoxSphereTemplate<Type>::GetBox() const
{
  return ezBoundingBoxTemplate<Type>(m_vCenter - m_vBoxHalfExtends, m_vCenter + m_vBoxHalfExtends);
}

template<typename Type>
EZ_FORCE_INLINE const ezBoundingSphereTemplate<Type> ezBoundingBoxSphereTemplate<Type>::GetSphere() const
{
  return ezBoundingSphereTemplate<Type>(m_vCenter, m_fSphereRadius);
}

template<typename Type>
void ezBoundingBoxSphereTemplate<Type>::ExpandToInclude(const ezBoundingBoxSphereTemplate& rhs)
{
  ezBoundingBoxTemplate<Type> box = GetBox();
  box.ExpandToInclude(rhs.GetBox());

  ezBoundingBoxSphereTemplate<Type> result(box);

  const float fSphereRadiusA = (m_vCenter - result.m_vCenter).GetLength() + m_fSphereRadius;
  const float fSphereRadiusB = (rhs.m_vCenter - result.m_vCenter).GetLength() + rhs.m_fSphereRadius;
  
  m_vCenter = result.m_vCenter;
  m_fSphereRadius = ezMath::Min(result.m_fSphereRadius, ezMath::Max(fSphereRadiusA, fSphereRadiusB));
  m_vBoxHalfExtends = result.m_vBoxHalfExtends;
}

template<typename Type>
void ezBoundingBoxSphereTemplate<Type>::Transform(const ezMat4Template<Type>& mTransform)
{
  ezMat3Template<Type> mAbsRotation = mTransform.GetRotationalPart();
  for (ezUInt32 i = 0; i < 9; ++i)
  {
    mAbsRotation.m_fElementsCM[i] = ezMath::Abs(mAbsRotation.m_fElementsCM[i]);
  }

  m_vCenter = mTransform.TransformPosition(m_vCenter);
  m_vBoxHalfExtends = mAbsRotation.TransformDirection(m_vBoxHalfExtends);

  const ezVec3Template<Type> Scale = mTransform.GetScalingFactors();
  m_fSphereRadius *= ezMath::Max(Scale.x, Scale.y, Scale.z);
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs)
{
  return lhs.m_vCenter == rhs.m_vCenter && lhs.m_vBoxHalfExtends == rhs.m_vBoxHalfExtends && lhs.m_fSphereRadius == rhs.m_fSphereRadius;
}

/// \brief Checks whether this box and the other are not identical.
template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs)
{
  return !(lhs == rhs);
}

