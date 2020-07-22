#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

struct ezSimdRandom
{
  static ezSimdVec4u UInt(const ezSimdVec4u& seed);

  static ezSimdVec4f FloatZeroToOne(const ezSimdVec4u& seed);

  static ezSimdVec4f FloatMinMax(const ezSimdVec4u& seed, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue);
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>
