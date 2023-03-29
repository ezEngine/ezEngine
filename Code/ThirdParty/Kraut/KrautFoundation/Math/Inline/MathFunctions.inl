#ifndef AE_FOUNDATION_MATH_MATH_INL
#define AE_FOUNDATION_MATH_MATH_INL

#include "../Math.h"

#include <math.h>
#include <cstdlib>

namespace AE_NS_FOUNDATION
{
  inline float aeMath::SinDeg (float f)
  {
    return (sinf (f * aeMath_DegToRad));
  }

  inline float aeMath::CosDeg (float f)
  {
    return (cosf (f * aeMath_DegToRad));
  }

  inline float aeMath::SinRad (float f)
  {
    return (sinf (f));
  }

  inline float aeMath::CosRad (float f)
  {
    return (cosf (f));
  }

  inline float aeMath::TanDeg (float f)
  {
    return (tanf (f * aeMath_DegToRad));
  }

  inline float aeMath::TanRad (float f)
  {
    return (tanf (f));
  }

  inline float aeMath::ASinDeg (float f)
  {
    return (asinf (f) * aeMath_RadToDeg);
  }

  inline float aeMath::ACosDeg  (float f)
  {
    return (acosf (f) * aeMath_RadToDeg);
  }

  inline float aeMath::ASinRad (float f)
  {
    return (asinf (f));
  }

  inline float aeMath::ACosRad (float f)
  {
    return (acosf (f));
  }

  inline float aeMath::ATanDeg (float f)
  {
    return (atanf (f) * aeMath_RadToDeg);
  }

  inline float aeMath::ATanRad (float f)
  {
    return (atanf (f));
  }

  inline float aeMath::ATan2Deg (float x, float y)
  {
    return (atan2f (x, y) * aeMath_RadToDeg);
  }

  inline float aeMath::ATan2Rad (float x, float y)
  {
    return (atan2f (x, y));
  }

  inline float aeMath::Exp (float f)
  {
    return (expf (f));
  }

  inline float aeMath::Ln (float f)
  {
    return (logf (f));
  }

  inline float aeMath::Log2 (float f)
  {
    return (log10f (f) / log10f (2.0f));
  }

  inline float aeMath::Log10 (float f)
  {
    return (log10f (f));
  }

  inline float aeMath::Log (float fBase, float f)
  {
    return (log10f (f) / log10f (fBase));
  }

  inline float aeMath::Pow2 (float f)
  {
    return (powf (2.0f, f));
  }

  inline float aeMath::Pow (float base, float exp)
  {
    return (powf (base, exp));
  }

  inline int aeMath::Pow2 (int i)
  {
    return (1 << i);
  }

  inline int aeMath::Pow (int base, int exp)
  {
    int res = 1;
    while (exp > 0)
    {
      res *= base;
      --exp;
    }
    return (res);
  }

  inline float aeMath::Root (float f, float NthRoot)
  {
    return (powf (f, 1.0f / NthRoot));
  }

  inline float aeMath::Sqrt (float f)
  {
    return (sqrtf (f));
  }

  inline float aeMath::Floor (float f)
  {
    return floorf (f);
  }

  inline float aeMath::Ceil (float f)
  {
    return ceilf (f);
  }

  inline float aeMath::Floor (float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Floor (fDivides);
    return (fFactor * fMultiple);
  }

  inline float aeMath::Ceil (float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Ceil (fDivides);
    return (fFactor * fMultiple);
  }

  inline aeInt32 aeMath::Floor (aeInt32 i, aeUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const aeInt32 iDivides = (i+1) / (aeInt32) uiMultiple;

      return ((iDivides-1) * uiMultiple);
    }

    const aeInt32 iDivides = i / (aeInt32) uiMultiple;
    return (iDivides * uiMultiple);
  }

  inline aeInt32 aeMath::Ceil (aeInt32 i, aeUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const aeInt32 iDivides = i / (aeInt32) uiMultiple;

      return (iDivides * uiMultiple);
    }

    aeInt32 iDivides = (i-1) / (aeInt32) uiMultiple;
    return ((iDivides+1) * uiMultiple);
  }

  inline float aeMath::Trunc (float f)
  {
    if (f > 0.0f)
      return Floor (f);

    return Ceil (f);
  }

  inline float aeMath::Fraction (float f)
  {
    return (f - Trunc (f));
  }

  inline aeInt32 aeMath::FloatToInt (float f)
  {
    return ((aeInt32) Trunc (f)); // same as trunc
  }

  inline float aeMath::Mod (float f, float div)
  {
    return (fmodf (f, div));
  }

  inline float aeMath::Invert (float f)
  {
    return (1.0f / f);
  }

  inline bool aeMath::IsOdd (aeInt32 i)
  {
    return ((i & 1) != 0);
  }

  inline bool aeMath::IsEven (aeInt32 i)
  {
    return ((i & 1) == 0);
  }

  inline float aeMath::SmoothStep (float x, float edge1, float edge2)
  {
    x = (x - edge1) / (edge2 - edge1);

    if (x <= 0.0f)
      return (0.0f);
    if (x >= 1.0f)
      return (1.0f);

    return (x * x * (3.0f - (2.0f * x)));
  }

  inline bool aeMath::IsPowerOf2 (aeInt32 value)
  {
    if (value < 1) 
      return (false);

    return ((value & (value - 1)) == 0);
  }

  inline bool aeMath::IsFloatEqual (float rhs, float lhs, float fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  inline bool aeMath::IsNaN (float f)
  {
    // all comparisons fail, if a float is NaN, even equality
    // volatile to make sure the compiler doesn't optimize this away
    volatile float f2 = f;
    volatile bool equal = (f2 == f2);
    return !equal;
  }

  inline bool aeMath::IsFinite (float f)
  {
    // if f is NaN both comparisons would fail
    // Thus returning false -> NaN is not a finite number

    // If f is Infinity or -Infinity the comparisons would also fail (it cannot be decided, but its not clearly smaller)
    return (f < aeMath::Infinity ()) && (f > -aeMath::Infinity ());
  }

  union aeIntToFloat
  {
    int i;
    float f;
  };

  inline float aeMath::Infinity () 
  {
    aeIntToFloat i2f;
    i2f.i = 0x7f800000; // bitwise representation of float infinity

    return i2f.f;
  } 
}

#endif



