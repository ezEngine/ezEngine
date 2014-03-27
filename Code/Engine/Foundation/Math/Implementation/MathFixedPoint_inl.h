#pragma once

#include <Foundation/Math/FixedPoint.h>

namespace ezMath
{
  #define FIXEDPOINT_OVERLOADS(Bits) \
    template<> EZ_FORCE_INLINE ezFixedPoint<Bits> BasicType<ezFixedPoint<Bits> >::MaxValue()        { return (ezFixedPoint<Bits>)((1 << (31 - Bits)) - 1); } \
    template<> EZ_FORCE_INLINE ezFixedPoint<Bits> BasicType<ezFixedPoint<Bits> >::SmallEpsilon()    { return (ezFixedPoint<Bits>) 0.0001; } \
    template<> EZ_FORCE_INLINE ezFixedPoint<Bits> BasicType<ezFixedPoint<Bits> >::DefaultEpsilon()  { return (ezFixedPoint<Bits>) 0.001; } \
    template<> EZ_FORCE_INLINE ezFixedPoint<Bits> BasicType<ezFixedPoint<Bits> >::LargeEpsilon()    { return (ezFixedPoint<Bits>) 0.01; } \
    template<> EZ_FORCE_INLINE ezFixedPoint<Bits> BasicType<ezFixedPoint<Bits> >::HugeEpsilon()     { return (ezFixedPoint<Bits>) 0.1; }

  FIXEDPOINT_OVERLOADS(1);
  FIXEDPOINT_OVERLOADS(2);
  FIXEDPOINT_OVERLOADS(3);
  FIXEDPOINT_OVERLOADS(4);
  FIXEDPOINT_OVERLOADS(5);
  FIXEDPOINT_OVERLOADS(6);
  FIXEDPOINT_OVERLOADS(7);
  FIXEDPOINT_OVERLOADS(8);
  FIXEDPOINT_OVERLOADS(9);
  FIXEDPOINT_OVERLOADS(10);
  FIXEDPOINT_OVERLOADS(11);
  FIXEDPOINT_OVERLOADS(12);
  FIXEDPOINT_OVERLOADS(13);
  FIXEDPOINT_OVERLOADS(14);
  FIXEDPOINT_OVERLOADS(15);
  FIXEDPOINT_OVERLOADS(16);
  FIXEDPOINT_OVERLOADS(17);
  FIXEDPOINT_OVERLOADS(18);
  FIXEDPOINT_OVERLOADS(19);
  FIXEDPOINT_OVERLOADS(20);
  FIXEDPOINT_OVERLOADS(21);
  FIXEDPOINT_OVERLOADS(22);
  FIXEDPOINT_OVERLOADS(23);
  FIXEDPOINT_OVERLOADS(24);
  FIXEDPOINT_OVERLOADS(25);
  FIXEDPOINT_OVERLOADS(26);
  FIXEDPOINT_OVERLOADS(27);
  FIXEDPOINT_OVERLOADS(28);
  FIXEDPOINT_OVERLOADS(29);
  FIXEDPOINT_OVERLOADS(30);
  //FIXEDPOINT_OVERLOADS(31);

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Floor(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) floor(f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Ceil(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) ceil(f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  inline ezFixedPoint<DecimalBits> Floor(ezFixedPoint<DecimalBits> f, ezFixedPoint<DecimalBits> fMultiple)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    ezFixedPoint<DecimalBits> fDivides = f / fMultiple;
    ezFixedPoint<DecimalBits> fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  template<ezUInt8 DecimalBits>
  inline ezFixedPoint<DecimalBits> Ceil(ezFixedPoint<DecimalBits> f, ezFixedPoint<DecimalBits> fMultiple)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    ezFixedPoint<DecimalBits> fDivides = f / fMultiple;
    ezFixedPoint<DecimalBits> fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Exp(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) exp(f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Ln(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) log(f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Log2(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) (log10(f.ToDouble()) / log10(2.0));
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Log10(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) log10(f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Log(ezFixedPoint<DecimalBits> fBase, ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) (log10(f.ToDouble()) / log10(fBase.ToDouble()));
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Pow2(ezFixedPoint<DecimalBits> f)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) pow(2.0, f.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Pow(ezFixedPoint<DecimalBits> base, ezFixedPoint<DecimalBits> exp)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) pow(base.ToDouble(), exp.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Root(ezFixedPoint<DecimalBits> f, ezFixedPoint<DecimalBits> NthRoot)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) pow(f.ToDouble(), 1.0 / NthRoot.ToDouble());
  }

  template<ezUInt8 DecimalBits>
  ezFixedPoint<DecimalBits> Sqrt(ezFixedPoint<DecimalBits> a)
  {
    return (ezFixedPoint<DecimalBits>) sqrt(a.ToDouble());
    //if (a <= ezFixedPoint<DecimalBits>(0))
    //  return ezFixedPoint<DecimalBits>(0);

    //ezFixedPoint<DecimalBits> x = a / 2;

    //for (ezUInt32 i = 0; i < 8; ++i)
    //{
    //  ezFixedPoint<DecimalBits> ax = a / x;
    //  ezFixedPoint<DecimalBits> xpax = x + ax;
    //  x = xpax / 2;
    //}

    //return x;
  }

  template<ezUInt8 DecimalBits>
  EZ_FORCE_INLINE ezFixedPoint<DecimalBits> Mod(ezFixedPoint<DecimalBits> f, ezFixedPoint<DecimalBits> div)
  {
    EZ_REPORT_FAILURE("This function is not really implemented yet.");

    return (ezFixedPoint<DecimalBits>) fmod(f.ToDouble(), div);
  }
}

