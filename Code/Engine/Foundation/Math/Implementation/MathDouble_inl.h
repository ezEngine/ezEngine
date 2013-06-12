#pragma once

namespace ezMath
{
  template<>
  inline bool BasicType<double>::SupportsInfinity()
  {
    return true;
  }

  template<>
  inline bool BasicType<double>::SupportsNaN()
  {
    return true;
  }

  inline bool IsFinite(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezInt64DoubleUnion i2f;
    i2f.f = value;
    return ((i2f.i & 0x7FF0000000000000LL) != 0x7FF0000000000000LL);
  }

  inline bool IsNaN(double value)
  {
    // Check the 11 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    ezInt64DoubleUnion i2f;
    i2f.f = value;

    return (((i2f.i & 0x7FF0000000000000LL) == 0x7FF0000000000000LL) && ((i2f.i & 0xFFFFFFFFFFFFFLL) != 0));
  }

  template<>
  inline double BasicType<double>::GetNaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001 

    ezInt64DoubleUnion i2f;
    i2f.i = 0x7FF0000000000042LL;

    return i2f.f;
  };

  template<>
  inline double BasicType<double>::GetInfinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

    ezInt64DoubleUnion i2f;
    i2f.i = 0x7FF0000000000000LL; // bitwise representation of double infinity (positive)

    return i2f.f;
  }

  template<>
  inline double BasicType<double>::MaxValue()
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

  EZ_FORCE_INLINE double SinDeg(double f)
  {
    return sin(DegToRad(f));
  }

  EZ_FORCE_INLINE double CosDeg(double f)
  {
    return cos(DegToRad(f));
  }

  EZ_FORCE_INLINE double SinRad(double f)
  {
    return sin(f);
  }

  EZ_FORCE_INLINE double CosRad(double f)
  {
    return cos(f);
  }

  EZ_FORCE_INLINE double TanDeg(double f)
  {
    return tan(DegToRad(f));
  }

  EZ_FORCE_INLINE double TanRad(double f)
  {
    return tan(f);
  }

  EZ_FORCE_INLINE double ASinDeg(double f)
  {
    return RadToDeg(asin(f));
  }

  EZ_FORCE_INLINE double ACosDeg(double f)
  {
    return RadToDeg(acos(f));
  }

  EZ_FORCE_INLINE double ASinRad(double f)
  {
    return asin(f);
  }

  EZ_FORCE_INLINE double ACosRad(double f)
  {
    return acos(f);
  }

  EZ_FORCE_INLINE double ATanDeg(double f)
  {
    return RadToDeg(atan(f));
  }

  EZ_FORCE_INLINE double ATanRad(double f)
  {
    return atan(f);
  }

  EZ_FORCE_INLINE double ATan2Deg(double x, double y)
  {
    return RadToDeg(atan2(x, y));
  }

  EZ_FORCE_INLINE double ATan2Rad(double x, double y)
  {
    return atan2(x, y);
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