#pragma once

#include <Foundation/Math/Vec3.h>

/// \brief Quaternions can be used to represent rotations in 3D space.
///
/// Quaternions are useful to represent 3D rotations, as they are smaller and more efficient than matrices
/// and can be concatenated easily, without having the 'Gimbal Lock' problem of Euler Angles.
/// Either use a full blown transformation (e.g. a 4x4 matrix) to represent a object, or use a Quaternion
/// bundled with a position vector, if (non-uniform) scale is not required.
/// Quaternions can also easily be interpolated (via Slerp).
/// This implementation also allows to convert back and forth between Quaternions and Matrices easily.
///
/// Quaternions have no 'IsIdentical' or 'IsEqual' function, as there can be different representations for the
/// same rotation, and it is rather difficult to check this. So to not convey any false notion of being equal
/// (or rather unequal), those functions are not provided.
template<typename Type>
class ezQuatTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  typedef Type ComponentType;

// *** Data ***
public:
  ezVec3Template<Type> v;
  Type w;

// *** Constructors ***
public:
  ezQuatTemplate(); // [tested]

  /// \brief For internal use. You should never construct quaternions this way.
  ezQuatTemplate(Type X, Type Y, Type Z, Type W); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  static const ezQuatTemplate<Type> IdentityQuaternion(); // [tested]

// *** Functions to create a quaternion ***
public:

  /// \brief Sets the Quaternion to the identity.
  void SetIdentity(); // [tested]
  
  /// \brief Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  void SetElements(Type X, Type Y, Type Z, Type W); // [tested]

  /// \brief Creates a quaternion from a rotation-axis and an angle.
  void SetFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle); // [tested]
  
  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  void SetShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo); // [tested]

  /// \brief Creates a quaternion from the given matrix.
  void SetFromMat3(const ezMat3Template<Type>& m); // [tested]

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  void SetSlerp(const ezQuatTemplate& qFrom, const ezQuatTemplate& qTo, Type t); // [tested]

// *** Common Functions ***
public:

  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle, that this quaternion rotates around.
  ezResult GetRotationAxisAndAngle(ezVec3Template<Type>& vAxis, ezAngle& angle) const; // [tested]
  
  /// \brief Returns the Quaternion as a matrix.
  const ezMat3Template<Type> GetAsMat3() const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  const ezMat4Template<Type> GetAsMat4() const; // [tested]
  
  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const ezQuatTemplate& qOther, float fEpsilon) const; // [tested]

// *** Operators ***
public:

  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  const ezQuatTemplate operator-() const; // [tested]

// *** Euler Angle Conversions ***
public:

  /// \brief Converts the quaternion to Euler angles
  void GetAsEulerAngles(ezAngle& out_Yaw, ezAngle& out_Pitch, ezAngle& out_Roll) const;

  /// \brief Sets the quaternion from Euler angles
  void SetFromEulerAngles(const ezAngle& Yaw, const ezAngle& Pitch, const ezAngle& Roll);
};

/// \brief Rotates v by q
template<typename Type>
const ezVec3Template<Type> operator* (const ezQuatTemplate<Type>& q, const ezVec3Template<Type>& v); // [tested]

/// \brief Concatenates the rotations of q1 and q2
template<typename Type>
const ezQuatTemplate<Type> operator* (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

template<typename Type>
bool operator== (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

template<typename Type>
bool operator!= (const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

#include <Foundation/Math/Implementation/Quat_inl.h>




