#pragma once

#include <Foundation/Math/Declarations.h>

/// \brief Implements fixed point arithmetic for fractional values.
///
/// Advantages over float and double are mostly that the computations are entirely integer-based and therefore
/// have a predictable (i.e. deterministic) result, independent from floating point settings, SSE support and
/// differences among CPUs.
/// Additionally fixed point arithmetic should be quite fast, compare to traditional floating point arithmetic
/// (not comparing it to SSE though).
/// With the template argument 'DecimalBits' you can specify how many bits are used for the fractional part.
/// I.e. a simple integer has zero DecimalBits. For a precision of about 1/1000 you need at least 10 DecimalBits
/// (1 << 10) == 1024.
/// Conversion between integer and fixed point is very fast (a shift), in contrast to float/int conversion.
///
/// If you are using ezFixedPoint to get guaranteed deterministic behavior, you should minimize the usage of
/// ezFixedPoint <-> float conversions. You can set ezFixedPoint variables from float constants, but you should
/// never put data into ezFixedPoint variables that was computed using floating point arithmetic (even if the
/// computations are simple and look harmless). Instead do all those computations with ezFixedPoint variables.
template <ezUInt8 DecimalBits>
class ezFixedPoint
{
public:
  /// \brief Default constructor does not do any initialization.
  EZ_ALWAYS_INLINE ezFixedPoint() = default; // [tested]

                                             /// \brief Construct from an integer.
  /* implicit */ ezFixedPoint(ezInt32 iIntVal) { *this = iIntVal; } // [tested]

                                                                    /// \brief Construct from a float.
  /* implicit */ ezFixedPoint(float fVal) { *this = fVal; } // [tested]

                                                            /// \brief Construct from a double.
  /* implicit */ ezFixedPoint(double fVal) { *this = fVal; } // [tested]

  /// \brief Assignment from an integer.
  const ezFixedPoint<DecimalBits>& operator=(ezInt32 iVal); // [tested]

  /// \brief Assignment from a float.
  const ezFixedPoint<DecimalBits>& operator=(float fVal); // [tested]

  /// \brief Assignment from a double.
  const ezFixedPoint<DecimalBits>& operator=(double fVal); // [tested]

  /// \brief Implicit conversion to int (the fractional part is dropped).
  ezInt32 ToInt() const; // [tested]

  /// \brief Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// \brief Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// \brief 'Equality' comparison.
  bool operator==(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue == rhs.m_iValue; } // [tested]

  /// \brief 'Inequality' comparison.
  bool operator!=(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue != rhs.m_iValue; } // [tested]

  /// \brief 'Less than' comparison.
  bool operator<(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue < rhs.m_iValue; } // [tested]

  /// \brief 'Greater than' comparison.
  bool operator>(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue > rhs.m_iValue; } // [tested]

  /// \brief 'Less than or equal' comparison.
  bool operator<=(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue <= rhs.m_iValue; } // [tested]

  /// \brief 'Greater than or equal' comparison.
  bool operator>=(const ezFixedPoint<DecimalBits>& rhs) const { return m_iValue >= rhs.m_iValue; } // [tested]


  const ezFixedPoint<DecimalBits> operator-() const { return ezFixedPoint<DecimalBits>(-m_iValue, true); }

  /// \brief += operator
  void operator+=(const ezFixedPoint<DecimalBits>& rhs) { m_iValue += rhs.m_iValue; } // [tested]

  /// \brief -= operator
  void operator-=(const ezFixedPoint<DecimalBits>& rhs) { m_iValue -= rhs.m_iValue; } // [tested]

  /// \brief *= operator
  void operator*=(const ezFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief /= operator
  void operator/=(const ezFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief *= operator with integers (more efficient)
  void operator*=(ezInt32 rhs) { m_iValue *= rhs; } // [tested]

  /// \brief /= operator with integers (more efficient)
  void operator/=(ezInt32 rhs) { m_iValue /= rhs; } // [tested]

  /// \brief Returns the underlying integer value. Mostly useful for serialization (or tests).
  ezInt32 GetRawValue() const { return m_iValue; }

  /// \brief Sets the underlying integer value. Mostly useful for serialization (or tests).
  void SetRawValue(ezInt32 iVal) { m_iValue = iVal; }

private:
  ezInt32 m_iValue;
};

template <ezUInt8 DecimalBits>
float ToFloat(ezFixedPoint<DecimalBits> f)
{
  return f.ToFloat();
}

// Additional operators:
// ezFixedPoint operator+ (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator- (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator* (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator/ (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator* (int, ezFixedPoint); // [tested]
// ezFixedPoint operator* (ezFixedPoint, int); // [tested]
// ezFixedPoint operator/ (ezFixedPoint, int); // [tested]

#include <Foundation/Math/Implementation/FixedPoint_inl.h>
