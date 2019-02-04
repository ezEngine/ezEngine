#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

struct ezSimdRandom
{
  static ezSimdVec4i Int(const ezSimdVec4i& seed);

  static ezSimdVec4f FloatZeroToOne(const ezSimdVec4i& seed);

  static ezSimdVec4f FloatMinMax(const ezSimdVec4i& seed, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue);
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>

