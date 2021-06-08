#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

struct ezSimdRandom
{
  static ezSimdVec4u UInt(const ezSimdVec4i& position, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());

  static ezSimdVec4f FloatZeroToOne(const ezSimdVec4i& position, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());

  static ezSimdVec4f FloatMinMax(const ezSimdVec4i& position, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>
