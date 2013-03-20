#pragma once

#include <Foundation/Math/Vec3.h>

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
  ezQuat() {}

  ezQuat(float X, float Y, float Z, float W);

// *** Functions to create a quaternion ***
public:

  /// \brief Sets the Quaternion to the identity.
  void SetIdentity();
  
  /// \brief Sets the Quaternion to these values.
  void SetElements(const ezVec3& V, float W);
  
  /// \brief Sets the Quaternion to these values.
  void SetElements(float X, float Y, float Z, float W);

  /// \brief Creates a quaternion from a rotation-axis and an angle.
  void SetFromAxisAndAngle(const ezVec3& vRotationAxis, float fAngle);
  
  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const ezVec3& vDirFrom, const ezVec3& vDirTo);

  /// \brief Creates a quaternion from the given matrix.
  void SetFromMat3(const ezMat3& m);

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const ezQuat& qFrom, const ezQuat& qTo, float t);

// *** Common Functions ***
public:

  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times.
  void Normalize();
  
  /// \brief Returns this quaternion normalized.
  const ezQuat GetNormalized() const;

  /// \brief Returns the rotation-axis, that this quaternion rotates around.
  const ezVec3 GetRotationAxis() const;
  
  /// \brief Returns the angle on the rotation-axis, that this quaternion rotates.
  float GetRotationAngle() const;

  /// \brief Returns the Quaternion as a matrix.
  const ezMat3 GetAsMat3() const;

  /// \brief Returns the Quaternion as a matrix.
  const ezMat4 GetAsMat4() const;

  /// \brief Returns whether this and the other quaternion are identical.
  bool IsIdentical(const ezQuat& rhs) const;

  /// \brief Returns whether this and the other quaternion are equal within a certain threshold.
  bool IsEqual(const ezQuat& rhs, float fEpsilon) const;

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(float fEpsilon = ezMath_DefaultEpsilon) const;

// *** Operators ***
public:

  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  const ezQuat operator-() const;
};

/// \brief Rotates v by q
const ezVec3 operator* (const ezQuat& q, const ezVec3& v);

/// \brief Concatenates the rotations of q1 and q2
const ezQuat operator* (const ezQuat& q1, const ezQuat& q2);

/// \brief Checks whether the two quaternions are identical.
bool operator== (const ezQuat& q1, const ezQuat& q2);

/// \brief Checks whether the two quaternions are not identical.
bool operator!= (const ezQuat& q1, const ezQuat& q2);


#include <Foundation/Math/Implementation/Quat_inl.h>




