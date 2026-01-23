#pragma once

#include <algorithm>
//#include <type_traits>

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

  EZ_ALWAYS_INLINE float Floor(float f)
  {
    return floorf(f);
  }

  EZ_ALWAYS_INLINE ezInt32 FloorToInt(float f)
  {
    return static_cast<ezInt32>(floorf(f));
  }

  EZ_ALWAYS_INLINE float Ceil(float f)
  {
    return ceilf(f);
  }

  EZ_ALWAYS_INLINE ezInt32 CeilToInt(float f)
  {
    return static_cast<ezInt32>(ceilf(f));
  }

  EZ_ALWAYS_INLINE float Round(float f)
  {
    return Floor(f + 0.5f);
  }

  EZ_ALWAYS_INLINE ezInt32 RoundToInt(float f)
  {
    return FloorToInt(f + 0.5f);
  }

  EZ_ALWAYS_INLINE float RoundToMultiple(float f, float fMultiple)
  {
    return Round(f / fMultiple) * fMultiple;
  }


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
  template <typename Type>
  EZ_ALWAYS_INLINE Type Sin(ezAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return sinf(a.GetRadian());
    else
      return sin(a.GetRadian());
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type Cos(ezAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return cosf(a.GetRadian());
    else
      return cos(a.GetRadian());
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type Tan(ezAngleTemplate<Type> a)
  {
    if constexpr (std::is_same_v<Type, float>)
      return tanf(a.GetRadian());
    else
      return tan(a.GetRadian());
  }
  
  template <typename Type>
  EZ_ALWAYS_INLINE ezAngleTemplate<Type> ASin(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return ezAngleTemplate<Type>::MakeFromRadian(asinf(f));
    else
      return ezAngleTemplate<Type>::MakeFromRadian(asin(f));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE ezAngleTemplate<Type> ACos(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return ezAngleTemplate<Type>::MakeFromRadian(acosf(f));
    else
      return ezAngleTemplate<Type>::MakeFromRadian(acos(f));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE ezAngleTemplate<Type> ATan(Type f)
  {
    if constexpr (std::is_same_v<Type, float>)
      return ezAngleTemplate<Type>::MakeFromRadian(atanf(f));
    else
      return ezAngleTemplate<Type>::MakeFromRadian(atan(f));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE ezAngleTemplate<Type> ATan2(Type y, Type x)
  {
    if constexpr (std::is_same_v<Type, float>)
      return ezAngleTemplate<Type>::MakeFromRadian(atan2f(y, x));
    else
      return ezAngleTemplate<Type>::MakeFromRadian(atan2(y, x));
  }

  EZ_ALWAYS_INLINE float Exp(float f)
  {
    return expf(f);
  }

  EZ_ALWAYS_INLINE float Ln(float f)
  {
    return logf(f);
  }

  EZ_ALWAYS_INLINE float Log2(float f)
  {
    return log2f(f);
  }

  EZ_ALWAYS_INLINE float Log10(float f)
  {
    return log10f(f);
  }

  EZ_ALWAYS_INLINE float Log(float fBase, float f)
  {
    return log10f(f) / log10f(fBase);
  }

  EZ_ALWAYS_INLINE float Pow2(float f)
  {
    return exp2f(f);
  }

  EZ_ALWAYS_INLINE float Pow(float fBase, float fExp)
  {
    return powf(fBase, fExp);
  }

  EZ_ALWAYS_INLINE float Root(float f, float fNthRoot)
  {
    return powf(f, 1.0f / fNthRoot);
  }

  EZ_ALWAYS_INLINE float Sqrt(float f)
  {
    return sqrtf(f);
  }

  EZ_ALWAYS_INLINE float Mod(float f, float fDiv)
  {
    return fmodf(f, fDiv);
  }
} // namespace ezMath
