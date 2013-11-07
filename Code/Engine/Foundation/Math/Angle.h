#pragma once

#include <Foundation/Math/Math.h>

/// \brief Float wrapper struct for a safe usage and conversions of angles.
/// 
/// Uses radian internally. Will <b>not</b> automatically keep its range between 0°-360° (0-2PI) but you can call NormalizeRange to do so.
class ezAngle
{
public:

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  template<typename Type>
  static EZ_FORCE_INLINE Type DegToRadMultiplier();

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  template<typename Type>
  static EZ_FORCE_INLINE Type RadToDegMultiplier();

  /// \brief Converts an angle in degree to radians.
  template<typename Type>
  static Type DegToRad(Type f); // [tested]

  /// \brief Converts an angle in radians to degree.
  template<typename Type>
  static Type RadToDeg(Type f); // [tested]

  /// \brief Creates an instance of ezAngle that was initialized from degree. (Performs a conversion)
  static ezAngle Degree(float fDegree);

  /// \brief Creates an instance of ezAngle that was initialized from radian. (No need for any conversion)
  static ezAngle Radian(float fRadian);



  EZ_DECLARE_POD_TYPE();

  /// \brief Standard constructor, initializing with 0.
  ezAngle() : m_fRadian(0.0f) {}

  /// \brief Returns the degree value. (Performs a conversion)
  float GetDegree() const; // [tested]

  /// \brief Returns the radian value. (No need for any conversion)
  float GetRadian() const; // [tested]

  /// \brief Brings the angle into the range of 0°-360°
  /// \see GetNormalizedRange()
  void NormalizeRange();

  /// \brief Returns an equivalent angle with range between 0°-360°
  /// \see NormalizeRange()
  ezAngle GetNormalizedRange(); // [tested]

  /// \brief Equality check with epsilon. Simple check without normalization, 360° != 0
  bool IsEqualSimple(ezAngle rhs, ezAngle epsilon); // [tested]

  /// \brief Equality check with epsilon that uses normalized angles. Will also recognize 360° == 0°
  bool IsEqualNormalized(ezAngle rhs, ezAngle epsilon); // [tested]

  // unary operators
  ezAngle operator - () const; // [tested]

  // arithmetic operators
  ezAngle operator + (ezAngle r) const; // [tested]
  ezAngle operator - (ezAngle r) const; // [tested]

  // compound assignment operators
  void operator += (ezAngle r); // [tested]
  void operator -= (ezAngle r); // [tested]

  // comparison
  bool operator == (const ezAngle& r) const; // [tested]
  bool operator != (const ezAngle& r) const; // [tested]

  // Note: relational operators on angles are not really possible - is 0° smaller or bigger than 359 degree?

private:
  /// \brief For internal use only.
  explicit ezAngle(float fRadian) : m_fRadian(fRadian) {}

  /// The ezRadian value
  float m_fRadian;

  /// Preventing an include circle by defining pi again (annoying, but unlikely to change ;)). Normally you should use ezMath::BasicType<Type>::Pi()
  template<typename Type>
  static Type Pi();
};

// Mathematical operators with float

/// \brief Returns f times angle a.
ezAngle operator* (ezAngle a, float f); // [tested]
/// \brief Returns f times angle a.
ezAngle operator* (float f, ezAngle a); // [tested]

/// \brief Returns f fraction of angle a.
ezAngle operator/ (ezAngle a, float f); // [tested]
/// \brief Returns f fraction of angle a.
ezAngle operator/ (float f, ezAngle a); // [tested]

#include <Foundation/Math/Implementation/Angle_inl.h>