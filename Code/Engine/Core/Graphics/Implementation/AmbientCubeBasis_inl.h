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
T ezAmbientCube<T>::Evaluate(const ezVec3& vNormal)
{
  ezVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] +
         vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}
