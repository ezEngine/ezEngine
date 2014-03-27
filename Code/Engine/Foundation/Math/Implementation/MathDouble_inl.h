#pragma once

namespace ezMath
{
  template<>
  EZ_FORCE_INLINE bool BasicType<double>::SupportsInfinity()
  {
    return true;
  }

  template<>
  EZ_FORCE_INLINE bool BasicType<double>::SupportsNaN()
  {
    return true;
  }

  EZ_FORCE_INLINE bool IsFinite(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezInt64DoubleUnion i2f;
    i2f.f = value;
    return ((i2f.i & 0x7FF0000000000000LL) != 0x7FF0000000000000LL);
  }

  EZ_FORCE_INLINE bool IsNaN(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezInt64DoubleUnion i2f;
    i2f.f = value;

    return (((i2f.i & 0x7FF0000000000000LL) == 0x7FF0000000000000LL) && ((i2f.i & 0xFFFFFFFFFFFFFLL) != 0));
  }

  template<>
  EZ_FORCE_INLINE double BasicType<double>::GetNaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 

    ezInt64DoubleUnion i2f;
    i2f.i = 0x7FF0000000000042LL;

    return i2f.f;
  };

  template<>
  EZ_FORCE_INLINE double BasicType<double>::GetInfinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

    ezInt64DoubleUnion i2f;
    i2f.i = 0x7FF0000000000000LL; // bitwise representation of double infinity (positive)

    return i2f.f;
  }

  template<>
  EZ_FORCE_INLINE double BasicType<double>::MaxValue()
  {
    return 1.7976931348623158e+307;
  }

  EZ_FORCE_INLINE double Floor(double f)
  {
    return floor(f);
  }

  EZ_FORCE_INLINE double Ceil(double f)
  {
    return ceil(f);
  }

  inline double Floor(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline double Ceil(double f, double fMultiple)
  {
    double fDivides = f / fMultiple;
    double fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  EZ_FORCE_INLINE double Exp(double f)
  {
    return exp(f);
  }

  EZ_FORCE_INLINE double Ln(double f)
  {
    return log(f);
  }

  EZ_FORCE_INLINE double Log2(double f)
  {
    return log10(f) / log10(2.0);
  }

  EZ_FORCE_INLINE double Log10(double f)
  {
    return log10(f);
  }

  EZ_FORCE_INLINE double Log(double fBase, double f)
  {
    return log10(f) / log10(fBase);
  }

  EZ_FORCE_INLINE double Pow2(double f)
  {
    return pow(2.0, f);
  }

  EZ_FORCE_INLINE double Pow(double base, double exp)
  {
    return pow(base, exp);
  }

  EZ_FORCE_INLINE double Root(double f, double NthRoot)
  {
    return pow(f, 1.0 / NthRoot);
  }

  EZ_FORCE_INLINE double Sqrt(double f)
  {
    return sqrt(f);
  }

  EZ_FORCE_INLINE double Mod(double f, double div)
  {
    return fmod(f, div);
  }
}

