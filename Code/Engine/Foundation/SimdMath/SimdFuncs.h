#pragma once

#include <Foundation/SimdMath/SimdVec4f.h>

struct ezSimdFuncs
{
  static ezSimdVec4f ASin(const ezSimdVec4f& f);
  static ezSimdVec4f ACos(const ezSimdVec4f& f);
  static ezSimdVec4f ATan(const ezSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdFuncs_inl.h>

