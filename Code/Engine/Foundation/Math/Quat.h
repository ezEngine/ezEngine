#pragma once

#include <Foundation/Math/Vec3.h>

/// \brief Quaternians can be used to represent rotations in 3D space.
///
/// Quaternions are useful to represent 3D rotations, as they are smaller and more efficient than matrices
/// and can be concatenated easily, without having the 'Gimbal Lock' problem of Euler Angles.
/// Either use a full blown transformation (e.g. a 4x4 matrix) to represent a object, or use a Quaternion
/// bundled with a position vector, if (non-uniform) scale is not required.
/// Quaternions can also easily be interpolated (via Slerp).
/// This implementation also allows to convert back and forth between Quaternins and Matrices easily.
///
/// Quaternions have no 'IsIdentical' or 'IsEqual' function, as there can be different representations for the
/// same rotation, and it is rather difficult to check this. So to not convey any false notion of being equal
/// (or rather inequal), those functions are not provided.
class EZ_FOUNDATION_DLL ezQuat
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

// *** Data ***
public:
  ezVec3 v;
  float w;

// *** Constructors ***
public:
  ezQuat(); // [tested]

  /// \brief For internal use. You should never construct quaternions this way.
  ezQuat(float X, float Y, float Z, float W); // [tested]

// *** Functions to create a quaternion ***
public:

  /// \brief Sets the Quaternion to the identity.
  void SetIdentity(); // [tested]
  
  /// \brief Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  void SetElements(float X, float Y, float Z, float W); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle.
  void SetFromAxisAndAngle(const ezVec3& vRotationAxis, float fAngle); // [tested]
  
  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const ezVec3& vDirFrom, const ezVec3& vDirTo); // [tested]

  /// \brief Creates a quaternion from the given matrix.
  void SetFromMat3(const ezMat3& m); // [tested]

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const ezQuat& qFrom, const ezQuat& qTo, float t); // [tested]

// *** Common Functions ***
public:

  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle, that this quaternion rotates around.
  ezResult GetRotationAxisAndAngle(ezVec3& vAxis, float& fAngle) const; // [tested]
  
  /// \brief Returns the Quaternion as a matrix.
  const ezMat3 GetAsMat3() const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  const ezMat4 GetAsMat4() const; // [tested]
  
  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(float fEpsilon = ezMath_DefaultEpsilon) const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const ezQuat& qOther, float fEpsilon) const; // [tested]

// *** Operators ***
public:

  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  const ezQuat operator-() const; // [tested]
};

/// \brief Rotates v by q
const ezVec3 operator* (const ezQuat& q, const ezVec3& v); // [tested]

/// \brief Concatenates the rotations of q1 and q2
const ezQuat operator* (const ezQuat& q1, const ezQuat& q2); // [tested]

#include <Foundation/Math/Implementation/Quat_inl.h>




