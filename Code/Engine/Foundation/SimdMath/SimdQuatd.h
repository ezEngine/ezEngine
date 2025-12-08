#pragma once

#include <Foundation/SimdMath/SimdMat4d.h>

class EZ_FOUNDATION_DLL ezSimdQuatd
{
public:
  EZ_DECLARE_POD_TYPE();

  ezSimdQuatd();                              // [tested]

  explicit ezSimdQuatd(const ezSimdVec4d& v); // [tested]

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  [[nodiscard]] static const ezSimdQuatd MakeIdentity(); // [tested]

  /// \brief Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an
  /// angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  [[nodiscard]] static ezSimdQuatd MakeFromElements(ezSimdDouble x, ezSimdDouble y, ezSimdDouble z, ezSimdDouble w); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle (angle is given in Radians or as an ezAngle)
  [[nodiscard]] static ezSimdQuatd MakeFromAxisAndAngle(const ezSimdVec4d& vRotationAxis, const ezSimdDouble& fAngle); // [tested]

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  [[nodiscard]] static ezSimdQuatd MakeShortestRotation(const ezSimdVec4d& vDirFrom, const ezSimdVec4d& vDirTo); // [tested]

  /// \brief Returns a quaternion that is the spherical linear interpolation of the other two.
  [[nodiscard]] static ezSimdQuatd MakeSlerp(const ezSimdQuatd& qFrom, const ezSimdQuatd& qTo, const ezSimdDouble& t); // [tested]

public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle (in Radians), that this quaternion rotates around.
  ezResult GetRotationAxisAndAngle(ezSimdVec4d& ref_vAxis, ezSimdDouble& ref_fAngle, const ezSimdDouble& fEpsilon = ezMath::DefaultEpsilon<double>()) const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  ezSimdMat4d GetAsMat4() const; // [tested]

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(const ezSimdDouble& fEpsilon = ezMath::DefaultEpsilon<float>()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const ezSimdQuatd& qOther, const ezSimdDouble& fEpsilon) const; // [tested]

public:
  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  [[nodiscard]] ezSimdQuatd operator-() const; // [tested]

  /// \brief Rotates v by q
  [[nodiscard]] ezSimdVec4d operator*(const ezSimdVec4d& v) const; // [tested]

  /// \brief Concatenates the rotations of q1 and q2
  [[nodiscard]] ezSimdQuatd operator*(const ezSimdQuatd& q2) const; // [tested]

  bool operator==(const ezSimdQuatd& q2) const;                    // [tested]
  bool operator!=(const ezSimdQuatd& q2) const;                    // [tested]

public:
  ezSimdVec4d m_v;
};

#include <Foundation/SimdMath/Implementation/SimdQuatd_inl.h>
