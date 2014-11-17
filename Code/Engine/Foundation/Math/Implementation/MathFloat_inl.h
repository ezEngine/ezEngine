#pragma once

#include <algorithm>

namespace ezMath
{
  template<>
  EZ_FORCE_INLINE bool BasicType<float>::SupportsInfinity()
  {
    return true;
  }

  template<>
  EZ_FORCE_INLINE bool BasicType<float>::SupportsNaN()
  {
    return true;
  }

  EZ_FORCE_INLINE bool IsFinite(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezIntFloatUnion i2f;
    i2f.f = value;
    return ((i2f.i & 0x7f800000) != 0x7f800000);
  }

  EZ_FORCE_INLINE bool IsNaN(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezIntFloatUnion i2f;
    i2f.f = value;

    return (((i2f.i & 0x7f800000) == 0x7f800000) && ((i2f.i & 0x7FFFFF) != 0));
  }

  template<>
  EZ_FORCE_INLINE float BasicType<float>::GetNaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1000 0000 0000 0000 0000 0001

    ezIntFloatUnion i2f;
    i2f.i = 0x7f800042;

    return i2f.f;
  };

  template<>
  EZ_FORCE_INLINE float BasicType<float>::GetInfinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1000 0000 0000 0000 0000 0000

    ezIntFloatUnion i2f;
    i2f.i = 0x7f800000; // bitwise representation of float infinity (positive)

    return i2f.f;
  }

  template<>
  EZ_FORCE_INLINE float BasicType<float>::MaxValue()
  {
    return 3.402823465e+38F;
  }

  EZ_FORCE_INLINE float Floor(float f)
  {
    return floorf(f);
  }

  EZ_FORCE_INLINE float Ceil(float f)
  {
    return ceilf(f);
  }

  inline float Floor(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline float Ceil(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  EZ_FORCE_INLINE float Sin(ezAngle a)
  {
    return sinf(a.GetRadian());
  }

  EZ_FORCE_INLINE float Cos(ezAngle a)
  {
    return cosf(a.GetRadian());
  }

  EZ_FORCE_INLINE float Tan(ezAngle a)
  {
    return tanf(a.GetRadian());
  }

  EZ_FORCE_INLINE ezAngle ASin(float f)
  {
    return ezAngle::Radian(asinf(f));
  }

  EZ_FORCE_INLINE ezAngle ACos(float f)
  {
    return ezAngle::Radian(acosf(f));
  }

  EZ_FORCE_INLINE ezAngle ATan(float f)
  {
    return ezAngle::Radian(atanf(f));
  }

  EZ_FORCE_INLINE ezAngle ATan2(float x, float y)
  {
    return ezAngle::Radian(atan2f(x, y));
  }

  EZ_FORCE_INLINE float Exp(float f)
  {
    return expf(f);
  }

  EZ_FORCE_INLINE float Ln(float f)
  {
    return logf(f);
  }

  EZ_FORCE_INLINE float Log2(float f)
  {
    return log10f(f) / log10f (2.0f);
  }

  EZ_FORCE_INLINE float Log10(float f)
  {
    return log10f(f);
  }

  EZ_FORCE_INLINE float Log(float fBase, float f)
  {
    return log10f(f) / log10f(fBase);
  }

  EZ_FORCE_INLINE float Pow2(float f)
  {
    return powf(2.0f, f);
  }

  EZ_FORCE_INLINE float Pow(float base, float exp)
  {
    return powf(base, exp);
  }

  EZ_FORCE_INLINE float Root(float f, float NthRoot)
  {
    return powf(f, 1.0f / NthRoot);
  }

  EZ_FORCE_INLINE float Sqrt(float f)
  {
    return sqrtf(f);
  }

  EZ_FORCE_INLINE float Mod(float f, float div)
  {
    return fmodf(f, div);
  }




}

