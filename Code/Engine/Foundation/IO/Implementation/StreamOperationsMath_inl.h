
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
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>

// ezVec2Template

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezVec2Template<Type>& Value)
{
  Stream << Value.x;
  Stream << Value.y;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezVec2Template<Type>& Value)
{
  Stream >> Value.x;
  Stream >> Value.y;
  return Stream;
}

// ezVec3Template

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezVec3Template<Type>& Value)
{
  Stream << Value.x;
  Stream << Value.y;
  Stream << Value.z;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezVec3Template<Type>& Value)
{
  Stream >> Value.x;
  Stream >> Value.y;
  Stream >> Value.z;
  return Stream;
}

// ezVec4Template

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezVec4Template<Type>& Value)
{
  Stream << Value.x;
  Stream << Value.y;
  Stream << Value.z;
  Stream << Value.w;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezVec4Template<Type>& Value)
{
  Stream >> Value.x;
  Stream >> Value.y;
  Stream >> Value.z;
  Stream >> Value.w;
  return Stream;
}

// ezMat3Template

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezMat3Template<Type>& Value)
{
  for (ezUInt32 i = 0; i < 9; ++i)
    Stream << Value.m_fElementsCM[i];

  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezMat3Template<Type>& Value)
{
  for (ezUInt32 i = 0; i < 9; ++i)
    Stream >> Value.m_fElementsCM[i];
  
  return Stream;
}

// ezMat4Template

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezMat4Template<Type>& Value)
{
  for (ezUInt32 i = 0; i < 16; ++i)
    Stream << Value.m_fElementsCM[i];

  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezMat4Template<Type>& Value)
{
  for (ezUInt32 i = 0; i < 16; ++i)
    Stream >> Value.m_fElementsCM[i];
  
  return Stream;
}

// ezPlaneTemplate

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezPlaneTemplate<Type>& Value)
{
  Stream << Value.m_vNormal.x;
  Stream << Value.m_vNormal.y;
  Stream << Value.m_vNormal.z;
  Stream << Value.m_fNegDistance;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezPlaneTemplate<Type>& Value)
{
  Stream >> Value.m_vNormal.x;
  Stream >> Value.m_vNormal.y;
  Stream >> Value.m_vNormal.z;
  Stream >> Value.m_fNegDistance;
  return Stream;
}

// ezQuatTemplate

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezQuatTemplate<Type>& Value)
{
  Stream << Value.v.x;
  Stream << Value.v.y;
  Stream << Value.v.z;
  Stream << Value.w;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezQuatTemplate<Type>& Value)
{
  Stream >> Value.v.x;
  Stream >> Value.v.y;
  Stream >> Value.v.z;
  Stream >> Value.w;
  return Stream;
}

// ezBoundingBoxTemplate

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezBoundingBoxTemplate<Type>& Value)
{
  Stream << Value.m_vMax;
  Stream << Value.m_vMin;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezBoundingBoxTemplate<Type>& Value)
{
  Stream >> Value.m_vMax;
  Stream >> Value.m_vMin;
  return Stream;
}

// ezBoundingSphereTemplate

template<typename Type>
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezBoundingSphereTemplate<Type>& Value)
{
  Stream << Value.m_vCenter;
  Stream << Value.m_fRadius;
  return Stream;
}

template<typename Type>
inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezBoundingSphereTemplate<Type>& Value)
{
  Stream >> Value.m_vCenter;
  Stream >> Value.m_fRadius;
  return Stream;
}

// ezColor
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezColor& Value)
{
  Stream << Value.r;
  Stream << Value.g;
  Stream << Value.b;
  Stream << Value.a;
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezColor& Value)
{
  Stream >> Value.r;
  Stream >> Value.g;
  Stream >> Value.b;
  Stream >> Value.a;
  return Stream;
}

// ezColor8Unorm
inline ezStreamWriterBase& operator << (ezStreamWriterBase& Stream, const ezColorLinearUB& Value)
{
  Stream << Value.r;
  Stream << Value.g;
  Stream << Value.b;
  Stream << Value.a;
  return Stream;
}

inline ezStreamReaderBase& operator >> (ezStreamReaderBase& Stream, ezColorLinearUB& Value)
{
  Stream >> Value.r;
  Stream >> Value.g;
  Stream >> Value.b;
  Stream >> Value.a;
  return Stream;
}

