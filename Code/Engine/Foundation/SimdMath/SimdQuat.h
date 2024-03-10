#pragma once

#include <Foundation/SimdMath/SimdMat4f.h>

class EZ_FOUNDATION_DLL ezSimdQuat
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdQuat();                              // [tested]

  explicit ezSimdQuat(const ezSimdVec4f& v); // [tested]

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const ezSimdQuat MakeIdentity(); // [tested]

  /// \brief Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an
  /// angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static ezSimdQuat MakeFromElements(ezSimdFloat x, ezSimdFloat y, ezSimdFloat z, ezSimdFloat w); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an ezAngle)
  [[nodiscard]] static ezSimdQuat MakeFromAxisAndAngle(const ezSimdVec4f& vRotationAxis, const ezSimdFloat& fAngle); // [tested]

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static ezSimdQuat MakeShortestRotation(const ezSimdVec4f& vDirFrom, const ezSimdVec4f& vDirTo); // [tested]

  /// \brief Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static ezSimdQuat MakeSlerp(const ezSimdQuat& qFrom, const ezSimdQuat& qTo, const ezSimdFloat& t); // [tested]

public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  ezResult GetRotationAxisAndAngle(ezSimdVec4f& ref_vAxis, ezSimdFloat& ref_fAngle, const ezSimdFloat& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  ezSimdMat4f GetAsMat4() const; // [tested]

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const ezSimdFloat& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const ezSimdQuat& qOther, const ezSimdFloat& fEpsilon) const; // [tested]

public:
  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] ezSimdQuat operator-() const; // [tested]

  /// \brief Rotates v by q
  [[nodiscard]] ezSimdVec4f operator*(const ezSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the rotations of q1 and q2
  [[nodiscard]] ezSimdQuat operator*(const ezSimdQuat& q2) const; // [tested]

  bool operator==(const ezSimdQuat& q2) const;                    // [tested]
  bool operator!=(const ezSimdQuat& q2) const;                    // [tested]

public:
  ezSimdVec4f m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuat_inl.h>
