#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

struct ezSimdMath
{
  static ezSimdVec4f Exp(const ezSimdVec4f& f);
  static ezSimdVec4f Ln(const ezSimdVec4f& f);
  static ezSimdVec4f Log2(const ezSimdVec4f& f);
  static ezSimdVec4i Log2i(const ezSimdVec4i& i);
  static ezSimdVec4f Log10(const ezSimdVec4f& f);
  static ezSimdVec4f Pow2(const ezSimdVec4f& f);

  static ezSimdVec4f Sin(const ezSimdVec4f& f);
  static ezSimdVec4f Cos(const ezSimdVec4f& f);
  static ezSimdVec4f Tan(const ezSimdVec4f& f);

  static ezSimdVec4f ASin(const ezSimdVec4f& f);
  static ezSimdVec4f ACos(const ezSimdVec4f& f);
  static ezSimdVec4f ATan(const ezSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
