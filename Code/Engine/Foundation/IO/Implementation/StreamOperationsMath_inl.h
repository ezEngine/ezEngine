#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

// ezVec2Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezVec2Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(ezVec2Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezVec2Template<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezVec2Template<Type>)) == sizeof(ezVec2Template<Type>), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezVec2Template<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezVec2Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezVec2Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec2Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezVec3Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezVec3Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(ezVec3Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezVec3Template<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezVec3Template<Type>)) == sizeof(ezVec3Template<Type>), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezVec3Template<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezVec3Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezVec3Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec3Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezVec4Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezVec4Template<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(ezVec4Template<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezVec4Template<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezVec4Template<Type>)) == sizeof(ezVec4Template<Type>), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezVec4Template<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezVec4Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezVec4Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec4Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezMat3Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezMat3Template<Type>& Value)
{
  stream.WriteBytes(Value.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezMat3Template<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(Value.m_fElementsCM, sizeof(Type) * 9) == sizeof(Type) * 9, "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezMat3Template<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezMat3Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezMat3Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezMat3Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezMat4Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezMat4Template<Type>& Value)
{
  stream.WriteBytes(Value.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezMat4Template<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(Value.m_fElementsCM, sizeof(Type) * 16) == sizeof(Type) * 16, "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezMat4Template<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezMat4Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezMat4Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezMat4Template<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezTransformTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezTransformTemplate<Type>& Value)
{
  stream << Value.m_qRotation;
  stream << Value.m_vPosition;
  stream << Value.m_vScale;

  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezTransformTemplate<Type>& Value)
{
  stream >> Value.m_qRotation;
  stream >> Value.m_vPosition;
  stream >> Value.m_vScale;

  return stream;
}

// ezPlaneTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezPlaneTemplate<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(ezPlaneTemplate<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezPlaneTemplate<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezPlaneTemplate<Type>)) == sizeof(ezPlaneTemplate<Type>), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezPlaneTemplate<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezPlaneTemplate<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezPlaneTemplate<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezQuatTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezQuatTemplate<Type>& Value)
{
  stream.WriteBytes(&Value, sizeof(ezQuatTemplate<Type>)).AssertSuccess();
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezQuatTemplate<Type>& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezQuatTemplate<Type>)) == sizeof(ezQuatTemplate<Type>), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezQuatTemplate<Type>* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezQuatTemplate<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezQuatTemplate<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezQuatTemplate<Type>) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezBoundingBoxTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezBoundingBoxTemplate<Type>& Value)
{
  stream << Value.m_vMax;
  stream << Value.m_vMin;
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezBoundingBoxTemplate<Type>& Value)
{
  stream >> Value.m_vMax;
  stream >> Value.m_vMin;
  return stream;
}

// ezBoundingSphereTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezBoundingSphereTemplate<Type>& Value)
{
  stream << Value.m_vCenter;
  stream << Value.m_fRadius;
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezBoundingSphereTemplate<Type>& Value)
{
  stream >> Value.m_vCenter;
  stream >> Value.m_fRadius;
  return stream;
}

// ezBoundingBoxSphereTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezBoundingBoxSphereTemplate<Type>& Value)
{
  stream << Value.m_vCenter;
  stream << Value.m_fSphereRadius;
  stream << Value.m_vBoxHalfExtends;
  return stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& stream, ezBoundingBoxSphereTemplate<Type>& Value)
{
  stream >> Value.m_vCenter;
  stream >> Value.m_fSphereRadius;
  stream >> Value.m_vBoxHalfExtends;
  return stream;
}

// ezColor
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezColor& Value)
{
  stream.WriteBytes(&Value, sizeof(ezColor)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezColor& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezColor)) == sizeof(ezColor), "End of stream reached.");
  return stream;
}

inline ezResult SerializeArray(ezStreamWriter& stream, const ezColor* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezColor) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezColor* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColor) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezColorGammaUB
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezColorGammaUB& Value)
{
  stream.WriteBytes(&Value, sizeof(ezColorGammaUB)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezColorGammaUB& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezColorGammaUB)) == sizeof(ezColorGammaUB), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezColorGammaUB* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezColorGammaUB) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezColorGammaUB* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColorGammaUB) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezAngle
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezAngle& Value)
{
  stream << Value.GetRadian();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezAngle& Value)
{
  float fRadian;
  stream >> fRadian;
  Value.SetRadian(fRadian);
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezAngle* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezAngle) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezAngle* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezAngle) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezColor8Unorm
inline ezStreamWriter& operator<<(ezStreamWriter& stream, const ezColorLinearUB& Value)
{
  stream.WriteBytes(&Value, sizeof(ezColorLinearUB)).AssertSuccess();
  return stream;
}

inline ezStreamReader& operator>>(ezStreamReader& stream, ezColorLinearUB& Value)
{
  EZ_VERIFY(stream.ReadBytes(&Value, sizeof(ezColorLinearUB)) == sizeof(ezColorLinearUB), "End of stream reached.");
  return stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& stream, const ezColorLinearUB* pArray, ezUInt64 uiCount)
{
  return stream.WriteBytes(pArray, sizeof(ezColorLinearUB) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& stream, ezColorLinearUB* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColorLinearUB) * uiCount;
  if (stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}
