#pragma once

#include <Foundation/Math/Math.h>

template<ezUInt8 DecimalBits>
const ezFixedPoint<DecimalBits>& ezFixedPoint<DecimalBits>::operator=(ezInt32 IntVal)
{
  m_Value = IntVal << DecimalBits;
  return *this;
}

template<ezUInt8 DecimalBits>
const ezFixedPoint<DecimalBits>& ezFixedPoint<DecimalBits>::operator=(float FloatVal)
{
  m_Value = (ezInt32) ezMath::Round(FloatVal * (1 << DecimalBits));
  return *this;
}

template<ezUInt8 DecimalBits>
const ezFixedPoint<DecimalBits>& ezFixedPoint<DecimalBits>::operator=(double FloatVal)
{
  m_Value = (ezInt32) ezMath::Round(FloatVal * (1 << DecimalBits));
  return *this;
}

template<ezUInt8 DecimalBits>
ezInt32 ezFixedPoint<DecimalBits>::ToInt() const
{
  return (ezInt32) (m_Value >> DecimalBits);
}

template<ezUInt8 DecimalBits>
float ezFixedPoint<DecimalBits>::ToFloat() const
{
  return (float) ((double) m_Value / (double) (1 << DecimalBits));
}

template<ezUInt8 DecimalBits>
double ezFixedPoint<DecimalBits>::ToDouble() const
{
  return ((double) m_Value / (double) (1 << DecimalBits));
}

template<ezUInt8 DecimalBits>
void ezFixedPoint<DecimalBits>::operator*= (const ezFixedPoint<DecimalBits>& rhs)
{
  // lhs and rhs are in N:M format (N Bits for the Integer part, M Bits for the fractional part)
  // after multiplication, it will be in 2N:2M format

  const ezInt64 TempLHS = m_Value;
  const ezInt64 TempRHS = rhs.m_Value;

  ezInt64 TempRes = TempLHS * TempRHS;

  // the lower DecimalBits Bits are nearly of no concern (we throw them away anyway), except for the upper most Bit
  // that is Bit '(DecimalBits - 1)' and its Bitmask is therefore '(1 << (DecimalBits - 1))'
  // If that Bit is set, then the lowest DecimalBits represent a value of more than '0.5' (of their range)
  // so '(TempRes & (1 << (DecimalBits - 1))) ' is either 0 or 1 depending on whether the lower DecimalBits Bits represent a value larger than 0.5 or not
  // we shift that Bit one to the left and add it to the original value and thus 'round up' the result
  TempRes += ((TempRes & (1 << (DecimalBits - 1))) << 1);

  TempRes >>= DecimalBits; // result format: 2N:M
  
  // the upper N Bits are thrown away during conversion from 64 Bit to 32 Bit
  m_Value = (ezInt32) TempRes;
}

template<ezUInt8 DecimalBits>
void ezFixedPoint<DecimalBits>::operator/= (const ezFixedPoint<DecimalBits>& rhs)
{
  ezInt64 TempLHS = m_Value;
  const ezInt64 TempRHS = rhs.m_Value;

  TempLHS <<= 31;

  ezInt64 TempRes = TempLHS / TempRHS;

  // same rounding concept as in multiplication
  TempRes += ((TempRes & (1 << (31 - DecimalBits - 1))) << 1);

  TempRes >>= (31 - DecimalBits);

  // here we throw away the upper 32 Bits again (not needed anymore)
  m_Value = (ezInt32) TempRes;

}


template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator+ (const ezFixedPoint<DecimalBits>& lhs, const ezFixedPoint<DecimalBits>& rhs)
{
  ezFixedPoint<DecimalBits> res = lhs;
  res += rhs;
  return res;
}

template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator- (const ezFixedPoint<DecimalBits>& lhs, const ezFixedPoint<DecimalBits>& rhs)
{
  ezFixedPoint<DecimalBits> res = lhs;
  res -= rhs;
  return res;
}

template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator* (const ezFixedPoint<DecimalBits>& lhs, const ezFixedPoint<DecimalBits>& rhs)
{
  ezFixedPoint<DecimalBits> res = lhs;
  res *= rhs;
  return res;
}

template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator/ (const ezFixedPoint<DecimalBits>& lhs, const ezFixedPoint<DecimalBits>& rhs)
{
  ezFixedPoint<DecimalBits> res = lhs;
  res /= rhs;
  return res;
}


template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator* (const ezFixedPoint<DecimalBits>& lhs, ezInt32 rhs)
{
  ezFixedPoint<DecimalBits> ret = lhs;
  ret *= rhs;
  return ret;
}

template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator* (ezInt32 lhs, const ezFixedPoint<DecimalBits>& rhs)
{
  ezFixedPoint<DecimalBits> ret = rhs;
  ret *= lhs;
  return ret;
}

template<ezUInt8 DecimalBits>
ezFixedPoint<DecimalBits> operator/ (const ezFixedPoint<DecimalBits>& lhs, ezInt32 rhs)
{
  ezFixedPoint<DecimalBits> ret = lhs;
  ret /= rhs;
  return ret;
}




