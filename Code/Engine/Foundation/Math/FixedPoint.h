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
template<ezUInt8 DecimalBits>
class ezFixedPoint
{
public:
  /// \brief Default constructor does not do any initialization.
  EZ_FORCE_INLINE ezFixedPoint() { } // [tested]

  /// \brief Construct from an integer.
  /* implicit */ ezFixedPoint(ezInt32 IntVal) { *this = IntVal; } // [tested]

  /// \brief Construct from a float.
  /* implicit */ ezFixedPoint(float FloatVal) { *this = FloatVal; } // [tested]

  /// \brief Construct from a double.
  /* implicit */ ezFixedPoint(double FloatVal) { *this = FloatVal; } // [tested]

  /// \brief Assignment from an integer.
  const ezFixedPoint<DecimalBits>& operator=(ezInt32 IntVal); // [tested]

  /// \brief Assignment from a float.
  const ezFixedPoint<DecimalBits>& operator=(float FloatVal); // [tested]

  /// \brief Assignment from a double.
  const ezFixedPoint<DecimalBits>& operator=(double FloatVal); // [tested]

  /// \brief Implicit conversion to int (the fractional part is dropped).
  ezInt32 ToInt() const; // [tested]

  /// \brief Implicit conversion to float.
  float ToFloat() const; // [tested]

  /// \brief Implicit conversion to double.
  double ToDouble() const; // [tested]

  /// \brief 'Equality' comparison.
  bool operator==(const ezFixedPoint<DecimalBits>& rhs) const { return m_Value == rhs.m_Value; } // [tested]

  /// \brief 'Inequality' comparison.
  bool operator!=(const ezFixedPoint<DecimalBits>& rhs) const { return m_Value != rhs.m_Value; } // [tested]

  /// \brief 'Less than' comparison.
  bool operator< (const ezFixedPoint<DecimalBits>& rhs) const { return m_Value <  rhs.m_Value; } // [tested]

  /// \brief 'Greater than' comparison.
  bool operator> (const ezFixedPoint<DecimalBits>& rhs) const { return m_Value >  rhs.m_Value; } // [tested]

  /// \brief 'Less than or equal' comparison.
  bool operator<=(const ezFixedPoint<DecimalBits>& rhs) const { return m_Value <= rhs.m_Value; } // [tested]

  /// \brief 'Greater than or equal' comparison.
  bool operator>=(const ezFixedPoint<DecimalBits>& rhs) const { return m_Value >= rhs.m_Value; } // [tested]


  const ezFixedPoint<DecimalBits> operator-() const { return ezFixedPoint<DecimalBits>(-m_Value, true); }

  /// \brief += operator
  void operator+= (const ezFixedPoint<DecimalBits>& rhs) { m_Value += rhs.m_Value; } // [tested]

  /// \brief -= operator
  void operator-= (const ezFixedPoint<DecimalBits>& rhs) { m_Value -= rhs.m_Value; } // [tested]

  /// \brief *= operator
  void operator*= (const ezFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief /= operator
  void operator/= (const ezFixedPoint<DecimalBits>& rhs); // [tested]

  /// \brief *= operator with integers (more efficient)
  void operator*= (ezInt32 rhs) { m_Value *= rhs; } // [tested]

  /// \brief /= operator with integers (more efficient)
  void operator/= (ezInt32 rhs) { m_Value /= rhs; } // [tested]

  /// \brief Returns the underlying integer value. Mostly useful for serialization (or tests).
  ezInt32 GetRawValue() const { return m_Value; }

  /// \brief Sets the underlying integer value. Mostly useful for serialization (or tests).
  void SetRawValue(ezInt32  val) { m_Value = val; }

private:

  ezInt32 m_Value;
};

template<ezUInt8 DecimalBits>
float ToFloat(ezFixedPoint<DecimalBits> f) { return f.ToFloat(); }

// Additional operators:
// ezFixedPoint operator+ (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator- (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator* (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator/ (ezFixedPoint, ezFixedPoint); // [tested]
// ezFixedPoint operator* (int, ezFixedPoint); // [tested]
// ezFixedPoint operator* (ezFixedPoint, int); // [tested]
// ezFixedPoint operator/ (ezFixedPoint, int); // [tested]

#include <Foundation/Math/Implementation/FixedPoint_inl.h>

