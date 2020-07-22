#pragma once

#include <algorithm>

namespace ezMath
{
  EZ_ALWAYS_INLINE bool IsFinite(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezIntFloatUnion i2f(value);
    return ((i2f.i & 0x7f800000u) != 0x7f800000u);
  }

  EZ_ALWAYS_INLINE bool IsNaN(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezIntFloatUnion i2f(value);
    return (((i2f.i & 0x7f800000u) == 0x7f800000u) && ((i2f.i & 0x7FFFFFu) != 0));
  }

  EZ_ALWAYS_INLINE float Floor(float f) { return floorf(f); }

  EZ_ALWAYS_INLINE float Ceil(float f) { return ceilf(f); }

  EZ_ALWAYS_INLINE float Round(float f) { return Floor(f + 0.5f); }

  EZ_ALWAYS_INLINE float RoundToMultiple(float f, float multiple) { return Round(f / multiple) * multiple; }


  inline float RoundDown(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline float RoundUp(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  EZ_ALWAYS_INLINE float Sin(ezAngle a) { return sinf(a.GetRadian()); }

  EZ_ALWAYS_INLINE float Cos(ezAngle a) { return cosf(a.GetRadian()); }

  EZ_ALWAYS_INLINE float Tan(ezAngle a) { return tanf(a.GetRadian()); }

  EZ_ALWAYS_INLINE ezAngle ASin(float f) { return ezAngle::Radian(asinf(f)); }

  EZ_ALWAYS_INLINE ezAngle ACos(float f) { return ezAngle::Radian(acosf(f)); }

  EZ_ALWAYS_INLINE ezAngle ATan(float f) { return ezAngle::Radian(atanf(f)); }

  EZ_ALWAYS_INLINE ezAngle ATan2(float x, float y) { return ezAngle::Radian(atan2f(x, y)); }

  EZ_ALWAYS_INLINE float Exp(float f) { return expf(f); }

  EZ_ALWAYS_INLINE float Ln(float f) { return logf(f); }

  EZ_ALWAYS_INLINE float Log2(float f) { return log2f(f); }

  EZ_ALWAYS_INLINE float Log10(float f) { return log10f(f); }

  EZ_ALWAYS_INLINE float Log(float fBase, float f) { return log10f(f) / log10f(fBase); }

  EZ_ALWAYS_INLINE float Pow2(float f) { return exp2f(f); }

  EZ_ALWAYS_INLINE float Pow(float base, float exp) { return powf(base, exp); }

  EZ_ALWAYS_INLINE float Root(float f, float NthRoot) { return powf(f, 1.0f / NthRoot); }

  EZ_ALWAYS_INLINE float Sqrt(float f) { return sqrtf(f); }

  EZ_ALWAYS_INLINE float Mod(float f, float div) { return fmodf(f, div); }
} // namespace ezMath
