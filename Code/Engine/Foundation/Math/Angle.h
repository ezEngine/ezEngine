#pragma once

/// \brief Float/double wrapper struct for a safe usage and conversions of angles.
///
/// Uses radian internally. Will <b>not</b> automatically keep its range between 0 degree - 360 degree (0 - 2PI) but you can call NormalizeRange to do
/// so.
template <typename Type>
class EZ_FOUNDATION_DLL ezAngleTemplate
{
public:
  EZ_DECLARE_POD_TYPE();


  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static EZ_ALWAYS_INLINE Type DegToRadMultiplier(); // [tested]

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static EZ_ALWAYS_INLINE Type RadToDegMultiplier(); // [tested]

  /// \brief Converts an angle in degree to radians.
  constexpr static Type DegToRad(Type f); // [tested]

  /// \brief Converts an angle in radians to degree.
  constexpr static Type RadToDeg(Type f); // [tested]

  /// \brief Returns a zero initialized angle. Same as a default constructed object.
  [[nodiscard]] constexpr static ezAngleTemplate<Type> MakeZero() { return ezAngleTemplate<Type>(); }

  /// \brief Creates an instance of ezAngleTemplate<Type> that was initialized from degree. (Performs a conversion)
  [[nodiscard]] constexpr static ezAngleTemplate<Type> MakeFromDegree(Type fDegree); // [tested]

  /// \brief Creates an instance of ezAngleTemplate<Type> that was initialized from radian. (No need for any conversion)
  [[nodiscard]] constexpr static ezAngleTemplate<Type> MakeFromRadian(Type fRadian); // [tested]

  /// \brief Standard constructor, initializing with 0.
  constexpr ezAngleTemplate<Type>()
    : m_fRadian(0.0f)
  {
  } // [tested]

  /// \brief For internal use only.
  constexpr explicit ezAngleTemplate<Type>(Type fRadian)
    : m_fRadian(fRadian)
  {
  }




  /// \brief Returns the degree value. (Performs a conversion)
  constexpr Type GetDegree() const; // [tested]

  /// \brief Returns the radian value. (No need for any conversion)
  constexpr Type GetRadian() const; // [tested]


  /// \brief Sets the radian value. (No need for any conversion)
  EZ_ALWAYS_INLINE void SetRadian(Type fRad) { m_fRadian = fRad; };

  /// \brief Brings the angle into the range of 0 degree - 360 degree
  /// \see GetNormalizedRange()
  void NormalizeRange(); // [tested]

  /// \brief Returns an equivalent angle with range between 0 degree - 360 degree
  /// \see NormalizeRange()
  ezAngleTemplate<Type> GetNormalizedRange() const; // [tested]

  /// \brief Computes the smallest angle between the two given angles. The angle will always be a positive value.
  /// \note The two angles must be in the same range. E.g. they should be either normalized or at least the absolute angle between them should not be
  /// more than 180 degree.
  constexpr static ezAngleTemplate<Type> AngleBetween(ezAngleTemplate<Type> a, ezAngleTemplate<Type> b); // [tested]

  /// \brief Equality check with epsilon. Simple check without normalization. 360 degree will equal 0 degree, but 720 will not.
  bool IsEqualSimple(ezAngleTemplate<Type> rhs, ezAngleTemplate<Type> epsilon) const; // [tested]

  /// \brief Equality check with epsilon that uses normalized angles. Will recognize 720 degree == 0 degree.
  bool IsEqualNormalized(ezAngleTemplate<Type> rhs, ezAngleTemplate<Type> epsilon) const; // [tested]

  // unary operators
  constexpr ezAngleTemplate<Type> operator-() const; // [tested]

  // arithmetic operators
  constexpr ezAngleTemplate<Type> operator+(ezAngleTemplate<Type> r) const; // [tested]
  constexpr ezAngleTemplate<Type> operator-(ezAngleTemplate<Type> r) const; // [tested]

  // compound assignment operators
  void operator+=(ezAngleTemplate<Type> r); // [tested]
  void operator-=(ezAngleTemplate<Type> r); // [tested]

  // comparison
  constexpr bool operator==(const ezAngleTemplate<Type>& r) const; // [tested]
  constexpr bool operator!=(const ezAngleTemplate<Type>& r) const; // [tested]

  // At least the < operator is implement to make clamping etc. work
  constexpr bool operator<(const ezAngleTemplate<Type>& r) const;
  constexpr bool operator>(const ezAngleTemplate<Type>& r) const;
  constexpr bool operator<=(const ezAngleTemplate<Type>& r) const;
  constexpr bool operator>=(const ezAngleTemplate<Type>& r) const;

  // Note: relational operators on angles are not really possible - is 0 degree smaller or bigger than 359 degree?

private:

  /// The ezRadian value
  Type m_fRadian;

  /// Preventing an include circle by defining pi again (annoying, but unlikely to change ;)). Normally you should use ezMath::Pi<Type>()
  constexpr static Type Pi();
};

// Mathematical operators with Type

/// \brief Returns f times angle a.
template <typename Type>
constexpr ezAngleTemplate<Type> operator*(const ezAngleTemplate<Type>& a, Type f); // [tested]
/// \brief Returns f times angle a.
template <typename Type>
constexpr ezAngleTemplate<Type> operator*(Type f, const ezAngleTemplate<Type>& a); // [tested]

/// \brief Returns the angle a divided by f.
template <typename Type>
constexpr ezAngleTemplate<Type> operator/(const ezAngleTemplate<Type>& a, Type f); // [tested]
/// \brief Returns the fraction of angle a divided by angle b.
template <typename Type>
constexpr Type operator/(const ezAngleTemplate<Type>& a, const ezAngleTemplate<Type>& b); // [tested]

#include <Foundation/Math/Math.h>

#include <Foundation/Math/Implementation/Angle_inl.h>
