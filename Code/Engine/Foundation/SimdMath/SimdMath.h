#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

struct ezSimdMath
{
  static ezSimdVec4f Sin(const ezSimdVec4f& f);
  static ezSimdVec4f Cos(const ezSimdVec4f& f);
  static ezSimdVec4f Tan(const ezSimdVec4f& f);

  static ezSimdVec4f ASin(const ezSimdVec4f& f);
  static ezSimdVec4f ACos(const ezSimdVec4f& f);
  static ezSimdVec4f ATan(const ezSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>

