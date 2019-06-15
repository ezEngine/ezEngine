#pragma once

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdFuncs::ASin(const ezSimdVec4f& f)
{
  return ezSimdVec4f(ezMath::BasicType<float>::Pi() * 0.5f) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdFuncs::ACos(const ezSimdVec4f& f)
{
  ezSimdVec4f x1 = f.Abs();
  ezSimdVec4f x2 = x1.CompMul(x1);
  ezSimdVec4f x3 = x2.CompMul(x1);

  ezSimdVec4f s = x1 * -0.2121144f + ezSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((ezSimdVec4f(1.0f) - x1).GetSqrt());

  return ezSimdVec4f::Select(f >= ezSimdVec4f::ZeroVector(), s, ezSimdVec4f(ezMath::BasicType<float>::Pi()) - s);
}

// 4th order hyperbolical approximation
// 7 * 10^-5 radians precision
// Reference : Efficient approximations for the arctangent function, Rajan, S. Sichun Wang Inkol, R. Joyal, A., May 2006
// static
EZ_FORCE_INLINE ezSimdVec4f ezSimdFuncs::ATan(const ezSimdVec4f& f)
{
  return f.CompMul(f.Abs() * -0.1784f - f.CompMul(f) * 0.0663f + ezSimdVec4f(1.0301f));
}
