
#pragma once

#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>

// ezVec2

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezVec2& Value)
{
  Stream.WriteDWordValue(&Value.x);
  Stream.WriteDWordValue(&Value.y);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezVec2& Value)
{
  Stream.ReadDWordValue(&Value.x);
  Stream.ReadDWordValue(&Value.y);
  return Stream;
}

// ezVec3

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezVec3& Value)
{
  Stream.WriteDWordValue(&Value.x);
  Stream.WriteDWordValue(&Value.y);
  Stream.WriteDWordValue(&Value.z);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezVec3& Value)
{
  Stream.ReadDWordValue(&Value.x);
  Stream.ReadDWordValue(&Value.y);
  Stream.ReadDWordValue(&Value.z);
  return Stream;
}

// ezVec4

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezVec4& Value)
{
  Stream.WriteDWordValue(&Value.x);
  Stream.WriteDWordValue(&Value.y);
  Stream.WriteDWordValue(&Value.z);
  Stream.WriteDWordValue(&Value.w);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezVec4& Value)
{
  Stream.ReadDWordValue(&Value.x);
  Stream.ReadDWordValue(&Value.y);
  Stream.ReadDWordValue(&Value.z);
  Stream.ReadDWordValue(&Value.w);
  return Stream;
}

// ezMat3

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezMat3& Value)
{
  for (ezUInt32 i = 0; i < 9; ++i)
    Stream.WriteDWordValue(&Value.m_fElementsCM[i]);

  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezMat3& Value)
{
  for (ezUInt32 i = 0; i < 9; ++i)
    Stream.ReadDWordValue(&Value.m_fElementsCM[i]);
  
  return Stream;
}

// ezMat4

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezMat4& Value)
{
  for (ezUInt32 i = 0; i < 16; ++i)
    Stream.WriteDWordValue(&Value.m_fElementsCM[i]);

  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezMat4& Value)
{
  for (ezUInt32 i = 0; i < 16; ++i)
    Stream.ReadDWordValue(&Value.m_fElementsCM[i]);
  
  return Stream;
}

// ezPlane

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezPlane& Value)
{
  Stream.WriteDWordValue(&Value.m_vNormal.x);
  Stream.WriteDWordValue(&Value.m_vNormal.y);
  Stream.WriteDWordValue(&Value.m_vNormal.z);
  Stream.WriteDWordValue(&Value.m_fNegDistance);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezPlane& Value)
{
  Stream.ReadDWordValue(&Value.m_vNormal.x);
  Stream.ReadDWordValue(&Value.m_vNormal.y);
  Stream.ReadDWordValue(&Value.m_vNormal.z);
  Stream.ReadDWordValue(&Value.m_fNegDistance);
  return Stream;
}

// ezQuat

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezQuat& Value)
{
  Stream.WriteDWordValue(&Value.v.x);
  Stream.WriteDWordValue(&Value.v.y);
  Stream.WriteDWordValue(&Value.v.z);
  Stream.WriteDWordValue(&Value.w);
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezQuat& Value)
{
  Stream.ReadDWordValue(&Value.v.x);
  Stream.ReadDWordValue(&Value.v.y);
  Stream.ReadDWordValue(&Value.v.z);
  Stream.ReadDWordValue(&Value.w);
  return Stream;
}

// ezBoundingBox

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezBoundingBox& Value)
{
  Stream << Value.m_vMax;
  Stream << Value.m_vMin;
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezBoundingBox& Value)
{
  Stream >> Value.m_vMax;
  Stream >> Value.m_vMin;
  return Stream;
}

// ezBoundingSphere

inline ezIBinaryStreamWriter& operator << (ezIBinaryStreamWriter& Stream, const ezBoundingSphere& Value)
{
  Stream << Value.m_vCenter;
  Stream << Value.m_fRadius;
  return Stream;
}

inline ezIBinaryStreamReader& operator >> (ezIBinaryStreamReader& Stream, ezBoundingSphere& Value)
{
  Stream >> Value.m_vCenter;
  Stream >> Value.m_fRadius;
  return Stream;
}