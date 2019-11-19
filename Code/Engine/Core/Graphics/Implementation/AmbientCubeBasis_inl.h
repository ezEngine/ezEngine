#pragma once

template <typename T>
EZ_ALWAYS_INLINE ezAmbientCube<T>::ezAmbientCube()
{
  ezMemoryUtils::ZeroFill(m_Values);
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezAmbientCube<T>::ezAmbientCube(const ezAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE void ezAmbientCube<T>::operator=(const ezAmbientCube<U>& other)
{
  for (ezUInt32 i = 0; i < ezAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
EZ_FORCE_INLINE bool ezAmbientCube<T>::operator==(const ezAmbientCube& other) const
{
  return ezMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezAmbientCube<T>::operator!=(const ezAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void ezAmbientCube<T>::AddSample(const ezVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += ezMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += ezMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += ezMath::Abs(vDir.z) * value;
}

template <typename T>
T ezAmbientCube<T>::Evaluate(const ezVec3& vNormal) const
{
  ezVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}
