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
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezVec2Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(ezVec2Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezVec2Template<Type>& ref_vValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(ezVec2Template<Type>)) == sizeof(ezVec2Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezVec2Template<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezVec2Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezVec2Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec2Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezVec3Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezVec3Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(ezVec3Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezVec3Template<Type>& ref_vValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(ezVec3Template<Type>)) == sizeof(ezVec3Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezVec3Template<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezVec3Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezVec3Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezVec4Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezVec4Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(ezVec4Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezVec4Template<Type>& ref_vValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(ezVec4Template<Type>)) == sizeof(ezVec4Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezVec4Template<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezVec4Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezVec4Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezVec4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezMat3Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezMat3Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezMat3Template<Type>& ref_mValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 9) == sizeof(Type) * 9, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezMat3Template<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezMat3Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezMat3Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezMat3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezMat4Template

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezMat4Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezMat4Template<Type>& ref_mValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 16) == sizeof(Type) * 16, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezMat4Template<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezMat4Template<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezMat4Template<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezMat4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezTransformTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezTransformTemplate<Type>& value)
{
  inout_stream << value.m_qRotation;
  inout_stream << value.m_vPosition;
  inout_stream << value.m_vScale;

  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezTransformTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_qRotation;
  inout_stream >> out_value.m_vPosition;
  inout_stream >> out_value.m_vScale;

  return inout_stream;
}

// ezPlaneTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezPlaneTemplate<Type>& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezPlaneTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezPlaneTemplate<Type>& out_value)
{
  EZ_VERIFY(inout_stream.ReadBytes(&out_value, sizeof(ezPlaneTemplate<Type>)) == sizeof(ezPlaneTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezPlaneTemplate<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezPlaneTemplate<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezPlaneTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezQuatTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezQuatTemplate<Type>& qValue)
{
  inout_stream.WriteBytes(&qValue, sizeof(ezQuatTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezQuatTemplate<Type>& ref_qValue)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_qValue, sizeof(ezQuatTemplate<Type>)) == sizeof(ezQuatTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezQuatTemplate<Type>* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezQuatTemplate<Type>) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezQuatTemplate<Type>* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezQuatTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezBoundingBoxTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezBoundingBoxTemplate<Type>& value)
{
  inout_stream << value.m_vMax;
  inout_stream << value.m_vMin;
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezBoundingBoxTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vMax;
  inout_stream >> out_value.m_vMin;
  return inout_stream;
}

// ezBoundingSphereTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezBoundingSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fRadius;
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezBoundingSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fRadius;
  return inout_stream;
}

// ezBoundingBoxSphereTemplate

template <typename Type>
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezBoundingBoxSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fSphereRadius;
  inout_stream << value.m_vBoxHalfExtents;
  return inout_stream;
}

template <typename Type>
inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezBoundingBoxSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fSphereRadius;
  inout_stream >> out_value.m_vBoxHalfExtents;
  return inout_stream;
}

// ezColor
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezColor& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezColor)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezColor& ref_value)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(ezColor)) == sizeof(ezColor), "End of stream reached.");
  return inout_stream;
}

inline ezResult SerializeArray(ezStreamWriter& inout_stream, const ezColor* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezColor) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezColor* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColor) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezColorGammaUB
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezColorGammaUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezColorGammaUB)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezColorGammaUB& ref_value)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(ezColorGammaUB)) == sizeof(ezColorGammaUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezColorGammaUB* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezColorGammaUB) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezColorGammaUB* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColorGammaUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezAngle
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezAngle& value)
{
  inout_stream << value.GetRadian();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezAngle& out_value)
{
  float fRadian;
  inout_stream >> fRadian;
  out_value.SetRadian(fRadian);
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezAngle* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezAngle) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezAngle* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezAngle) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}


// ezColor8Unorm
inline ezStreamWriter& operator<<(ezStreamWriter& inout_stream, const ezColorLinearUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(ezColorLinearUB)).AssertSuccess();
  return inout_stream;
}

inline ezStreamReader& operator>>(ezStreamReader& inout_stream, ezColorLinearUB& ref_value)
{
  EZ_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(ezColorLinearUB)) == sizeof(ezColorLinearUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
ezResult SerializeArray(ezStreamWriter& inout_stream, const ezColorLinearUB* pArray, ezUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(ezColorLinearUB) * uiCount);
}

template <typename Type>
ezResult DeserializeArray(ezStreamReader& inout_stream, ezColorLinearUB* pArray, ezUInt64 uiCount)
{
  const ezUInt64 uiNumBytes = sizeof(ezColorLinearUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return EZ_SUCCESS;

  return EZ_FAILURE;
}
