#pragma once

#include <algorithm>
#include <utility>

EZ_FORCE_INLINE float ezMath::e()                   
{ 
  return ezMath_e;              
}

EZ_FORCE_INLINE float ezMath::Pi()                  
{ 
  return ezMath_Pi;             
}

EZ_FORCE_INLINE float ezMath::FloatMax_Pos()        
{ 
  return ezMath_FloatMax_Pos;   
}

EZ_FORCE_INLINE float ezMath::FloatMax_Neg()        
{ 
  return ezMath_FloatMax_Neg;   
}

EZ_FORCE_INLINE float ezMath::DegToRad(float f)     
{ 
  return ezMath_DegToRad * f;   
}

EZ_FORCE_INLINE float ezMath::RadToDeg(float f)     
{ 
  return ezMath_RadToDeg * f;   
}

EZ_FORCE_INLINE float ezMath::Infinity() 
{
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  // INF = 0111 1111 1000 0000 0000 0000 0000 0000

  ezIntFloatUnion i2f;
  i2f.i = 0x7f800000; // bitwise representation of float infinity

  return i2f.f;
} 

EZ_FORCE_INLINE float ezMath::NaN()
{
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  // NaN = 0111 1111 1000 0000 0000 0000 0000 0001

  ezIntFloatUnion i2f;
  i2f.i = 0x7f800001;

  return i2f.f;
}

EZ_FORCE_INLINE float ezMath::SinDeg(float f)
{
  return (sinf (f * ezMath_DegToRad));
}

EZ_FORCE_INLINE float ezMath::CosDeg(float f)
{
  return (cosf (f * ezMath_DegToRad));
}

EZ_FORCE_INLINE float ezMath::SinRad(float f)
{
  return (sinf (f));
}

EZ_FORCE_INLINE float ezMath::CosRad(float f)
{
  return (cosf (f));
}

EZ_FORCE_INLINE float ezMath::TanDeg(float f)
{
  return (tanf (f * ezMath_DegToRad));
}

EZ_FORCE_INLINE float ezMath::TanRad(float f)
{
  return (tanf (f));
}

EZ_FORCE_INLINE float ezMath::ASinDeg(float f)
{
  return (asinf (f) * ezMath_RadToDeg);
}

EZ_FORCE_INLINE float ezMath::ACosDeg(float f)
{
  return (acosf (f) * ezMath_RadToDeg);
}

EZ_FORCE_INLINE float ezMath::ASinRad(float f)
{
  return (asinf (f));
}

EZ_FORCE_INLINE float ezMath::ACosRad(float f)
{
  return (acosf (f));
}

EZ_FORCE_INLINE float ezMath::ATanDeg(float f)
{
  return (atanf (f) * ezMath_RadToDeg);
}

EZ_FORCE_INLINE float ezMath::ATanRad(float f)
{
  return (atanf (f));
}

EZ_FORCE_INLINE float ezMath::ATan2Deg(float x, float y)
{
  return (atan2f (x, y) * ezMath_RadToDeg);
}

EZ_FORCE_INLINE float ezMath::ATan2Rad(float x, float y)
{
  return (atan2f (x, y));
}

EZ_FORCE_INLINE float ezMath::Exp(float f)
{
  return (expf (f));
}

EZ_FORCE_INLINE float ezMath::Ln(float f)
{
  return (logf (f));
}

EZ_FORCE_INLINE float ezMath::Log2(float f)
{
  return (log10f (f) / log10f (2.0f));
}

inline ezUInt32 ezMath::Log2i(ezUInt32 val) 
{
  ezInt32 ret = -1;
  while (val != 0) 
  {
    val >>= 1;
    ret++;
  }

  return (ezUInt32) ret;
}


EZ_FORCE_INLINE float ezMath::Log10(float f)
{
  return (log10f (f));
}

EZ_FORCE_INLINE float ezMath::Log(float fBase, float f)
{
  return (log10f (f) / log10f (fBase));
}

EZ_FORCE_INLINE float ezMath::Pow2(float f)
{
  return (powf (2.0f, f));
}

EZ_FORCE_INLINE float ezMath::Pow(float base, float exp)
{
  return (powf (base, exp));
}

EZ_FORCE_INLINE int ezMath::Pow2(int i)
{
  return (1 << i);
}

inline int ezMath::Pow(int base, int exp)
{
  int res = 1;
  while (exp > 0)
  {
    res *= base;
    --exp;
  }
  return (res);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Square(T f)
{ 
  return (f * f); 
}

EZ_FORCE_INLINE float ezMath::Root(float f, float NthRoot)
{
  return (powf (f, 1.0f / NthRoot));
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Sign(T f) 
{
  return (f < 0 ? T (-1) : f > 0 ? T (1) : 0);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Abs(T f)
{
  return (f < 0 ? -f : f);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Min(T f1, T f2)
{
  return (f1 < f2 ? f1 : f2);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Min(T f1, T f2, T f3)
{
  return Min (Min (f1, f2), f3);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Min(T f1, T f2, T f3, T f4)
{
  return Min (Min (f1, f2), Min (f3, f4));
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Max(T f1, T f2)
{
  return (f1 > f2 ? f1 : f2);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Max(T f1, T f2, T f3)
{
  return Max (Max (f1, f2), f3);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Max(T f1, T f2, T f3, T f4)
{
  return Max (Max (f1, f2), Max (f3, f4));
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Clamp(T value, T min_val, T max_val) 
{ 
  if (value < min_val) return (min_val); 
  if (value > max_val) return (max_val);	
  return (value);	
}

EZ_FORCE_INLINE float ezMath::Sqrt(float f)
{
  return (sqrtf (f));
}

EZ_FORCE_INLINE float ezMath::Floor(float f)
{
  return floorf (f);
}

EZ_FORCE_INLINE float ezMath::Ceil(float f)
{
  return ceilf (f);
}

inline float ezMath::Floor(float f, float fMultiple)
{
  float fDivides = f / fMultiple;
  float fFactor = Floor (fDivides);
  return (fFactor * fMultiple);
}

inline float ezMath::Ceil(float f, float fMultiple)
{
  float fDivides = f / fMultiple;
  float fFactor = Ceil (fDivides);
  return (fFactor * fMultiple);
}

inline ezInt32 ezMath::Floor(ezInt32 i, ezUInt32 uiMultiple)
{
  if (i < 0)
  {
    const ezInt32 iDivides = (i+1) / (ezInt32) uiMultiple;

    return ((iDivides-1) * uiMultiple);
  }

  const ezInt32 iDivides = i / (ezInt32) uiMultiple;
  return (iDivides * uiMultiple);
}

inline ezInt32 ezMath::Ceil(ezInt32 i, ezUInt32 uiMultiple)
{
  if (i < 0)
  {
    const ezInt32 iDivides = i / (ezInt32) uiMultiple;

    return (iDivides * uiMultiple);
  }

  ezInt32 iDivides = (i-1) / (ezInt32) uiMultiple;
  return ((iDivides+1) * uiMultiple);
}

EZ_FORCE_INLINE float ezMath::Trunc(float f)
{
  if (f > 0.0f)
    return Floor (f);

  return Ceil (f);
}

EZ_FORCE_INLINE float ezMath::Round(float f)
{
  return ezMath::Floor(f + 0.5f);
}

EZ_FORCE_INLINE float ezMath::Round(float f, float fRoundTo)
{
  return Round (f / fRoundTo) * fRoundTo;
}

EZ_FORCE_INLINE float ezMath::Fraction(float f)
{
  return (f - Trunc (f));
}

EZ_FORCE_INLINE ezInt32 ezMath::FloatToInt(float f)
{
  return ((ezInt32) Trunc (f)); // same as trunc
}

EZ_FORCE_INLINE float ezMath::Mod(float f, float div)
{
  return (fmodf (f, div));
}

EZ_FORCE_INLINE float ezMath::Invert(float f)
{
  return (1.0f / f);
}

EZ_FORCE_INLINE bool ezMath::IsOdd(ezInt32 i)
{
  return ((i & 1) != 0);
}

EZ_FORCE_INLINE bool ezMath::IsEven(ezInt32 i)
{
  return ((i & 1) == 0);
}

template <typename T>
EZ_FORCE_INLINE void ezMath::Swap(T& f1, T& f2)
{ 
  std::swap(f1, f2);
}

template <typename T>
EZ_FORCE_INLINE T ezMath::Lerp(T f1, T f2, float factor) 	
{
  EZ_ASSERT ((factor >= -ezMath_DefaultEpsilon) && (factor <= 1.0f + ezMath_DefaultEpsilon), "ezMath::lerp: factor %.2f is not in the range [0; 1]", factor);

  return ((T) (f1 * (1.0f - factor) + f2 * factor));
}

///  Returns 0, if value < edge, and 1, if value >= edge.
template <typename T>
EZ_FORCE_INLINE T ezMath::Step(T value, T edge)
{
  return (value >= edge ? T (1) : T (0));
}


inline float ezMath::SmoothStep(float x, float edge1, float edge2)
{
  x = (x - edge1) / (edge2 - edge1);

  if (x <= 0.0f)
    return (0.0f);
  if (x >= 1.0f)
    return (1.0f);

  return (x * x * (3.0f - (2.0f * x)));
}

EZ_FORCE_INLINE bool ezMath::IsPowerOf2(ezInt32 value)
{
  if (value < 1) 
    return false;

  return ((value & (value - 1)) == 0);
}

EZ_FORCE_INLINE bool ezMath::IsFloatEqual(float rhs, float lhs, float fEpsilon)
{
  EZ_ASSERT (fEpsilon >= 0.0f, "Epsilon may not be negativ.");

  return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
}

EZ_FORCE_INLINE bool ezMath::IsNaN(float f)
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  ezIntFloatUnion i2f;
  i2f.f = f;

  return (((i2f.i & 0x7f800000) == 0x7f800000) && ((i2f.i & 0x7FFFFF) != 0));
}

EZ_FORCE_INLINE bool ezMath::IsFinite(float f)
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  ezIntFloatUnion i2f;
  i2f.f = f;
  return ((i2f.i & 0x7f800000) != 0x7f800000);
}

template <typename T>
inline bool ezMath::IsInRange(T Value, T MinVal, T MaxVal)
{
  if (MinVal < MaxVal)
    return (Value >= MinVal) && (Value <= MaxVal);
  else
    return (Value <= MinVal) && (Value >= MaxVal);
}

inline bool ezMath::IsZero(float f, float fEpsilon)
{
  EZ_ASSERT (fEpsilon >= 0.0f, "Epsilon may not be negativ.");

  return ((f >= -fEpsilon) && (f <= fEpsilon));
}


