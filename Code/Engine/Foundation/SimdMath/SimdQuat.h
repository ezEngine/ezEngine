#pragma once

#include <Foundation/SimdMath/SimdMat4f.h>

class EZ_FOUNDATION_DLL ezSimdQuat
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdQuat(); // [tested]

  ezSimdQuat(const ezSimdVec4f& v); // [tested]

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  static ezSimdQuat IdentityQuaternion(); // [tested]

public:
  /// \brief Sets the Quaternion to the identity.
  void SetIdentity(); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an ezAngle)
  void SetFromAxisAndAngle(const ezSimdVec4f& vRotationAxis, const ezSimdFloat& angle); // [tested]

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const ezSimdVec4f& vDirFrom, const ezSimdVec4f& vDirTo); // [tested]

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const ezSimdQuat& qFrom, const ezSimdQuat& qTo, const ezSimdFloat& t); // [tested]

public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  ezResult GetRotationAxisAndAngle(ezSimdVec4f& vAxis, ezSimdFloat& angle, const ezSimdFloat& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

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
  ezSimdQuat operator-() const; // [tested]

  /// \brief Rotates v by q
  ezSimdVec4f operator*(const ezSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the rotations of q1 and q2
  ezSimdQuat operator*(const ezSimdQuat& q2) const; // [tested]

  bool operator==(const ezSimdQuat& q2) const; // [tested]
  bool operator!=(const ezSimdQuat& q2) const; // [tested]

public:
  ezSimdVec4f m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuat_inl.h>

