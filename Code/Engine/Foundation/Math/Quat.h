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
template <typename Type>
class ezQuatTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  ezVec3Template<Type> v;
  Type w;

  // *** Constructors ***
public:
  ezQuatTemplate(); // [tested]

  /// \brief For internal use. You should never construct quaternions this way.
  ezQuatTemplate(Type x, Type y, Type z, Type w); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Static function that returns a quaternion that represents the identity rotation (none).
  static const ezQuatTemplate<Type> MakeIdentity(); // [tested]
  /*[[deprecated("Use ezQuat::MakeIdentity() instead.")]]*/ static const ezQuatTemplate<Type> IdentityQuaternion() { return ezQuatTemplate<Type>::MakeIdentity(); }

  // *** Functions to create a quaternion ***
public:
  /// \brief Sets the Quaternion to the identity.
  /*[[deprecated("Use ezQuat::MakeIdentity() instead.")]]*/ void SetIdentity(); // [tested]

  /// \brief Sets the individual elements of the quaternion directly. Note that x,y,z do NOT represent a rotation axis, and w does NOT represent an
  /// angle.
  ///
  /// Use this function only if you have good understanding of quaternion math and know exactly what you are doing.
  static ezQuatTemplate<Type> MakeFromElements(Type x, Type y, Type z, Type w); // [tested]
  /*[[deprecated("Use ezQuat::MakeFromElements() instead.")]]*/ void SetElements(Type inX, Type inY, Type inZ, Type inW) { *this = MakeFromElements(inX, inY, inZ, inW); }

  /// \brief Creates a quaternion from a rotation-axis and an angle.
  static ezQuatTemplate<Type> MakeFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle); // [tested]
  /*[[deprecated("Use ezQuat::MakeFromAxisAndAngle() instead.")]]*/ void SetFromAxisAndAngle(const ezVec3Template<Type>& vRotationAxis, ezAngle angle) { *this = MakeFromAxisAndAngle(vRotationAxis, angle); }

  /// \brief Creates a quaternion, that rotates through the shortest arc from "vDirFrom" to "vDirTo".
  static ezQuatTemplate<Type> MakeShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo); // [tested]
  /*[[deprecated("Use ezQuat::MakeShortestRotation() instead.")]]*/ void SetShortestRotation(const ezVec3Template<Type>& vDirFrom, const ezVec3Template<Type>& vDirTo) { *this = MakeShortestRotation(vDirFrom, vDirTo); }

  /// \brief Creates a quaternion from the given matrix.
  static ezQuatTemplate<Type> MakeFromMat3(const ezMat3Template<Type>& m); // [tested]
  /*[[deprecated("Use ezQuat::MakeFromMat3() instead.")]]*/ void SetFromMat3(const ezMat3Template<Type>& m) { *this = MakeFromMat3(m); }

  /// \brief Reconstructs a rotation quaternion from a matrix that may contain scaling and mirroring.
  ///
  /// In skeletal animation it is possible that matrices with mirroring are used, that need to be converted to a
  /// proper quaternion, even though a rotation with mirroring can't be represented by a quaternion.
  /// This function reconstructs a valid quaternion from such matrices. Obviously the mirroring information gets lost,
  /// but it is typically not needed any further anway.
  void ReconstructFromMat3(const ezMat3Template<Type>& m);

  /// \brief Reconstructs a rotation quaternion from a matrix that may contain scaling and mirroring.
  ///
  /// \sa ReconstructFromMat3()
  void ReconstructFromMat4(const ezMat4Template<Type>& m);

  /// \brief Sets this quaternion to be the spherical linear interpolation of the other two.
  static ezQuatTemplate<Type> MakeSlerp(const ezQuatTemplate& qFrom, const ezQuatTemplate& qTo, Type t); // [tested]
  /*[[deprecated("Use ezQuat::MakeSlerp() instead.")]]*/ void SetSlerp(const ezQuatTemplate& qFrom, const ezQuatTemplate& qTo, Type t) { *this = MakeSlerp(qFrom, qTo, t); }

  // *** Common Functions ***
public:
  /// \brief Normalizes the quaternion to unit length. ALL rotation-quaternions should be normalized at all times (automatically).
  void Normalize(); // [tested]

  /// \brief Returns the rotation-axis and angle, that this quaternion rotates around.
  void GetRotationAxisAndAngle(ezVec3Template<Type>& out_vAxis, ezAngle& out_angle, Type fEpsilon = ezMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  const ezMat3Template<Type> GetAsMat3() const; // [tested]

  /// \brief Returns the Quaternion as a matrix.
  const ezMat4Template<Type> GetAsMat4() const; // [tested]

  /// \brief Checks whether all components are neither NaN nor infinite and that the quaternion is normalized.
  bool IsValid(Type fEpsilon = ezMath::DefaultEpsilon<Type>()) const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Determines whether \a this and \a qOther represent the same rotation. This is a rather slow operation.
  ///
  /// Currently it fails when one of the given quaternions is identity (so no rotation, at all), as it tries to
  /// compare rotation axis' and angles, which is undefined for the identity quaternion (also there are infinite
  /// representations for 'identity', so it's difficult to check for it).
  bool IsEqualRotation(const ezQuatTemplate& qOther, Type fEpsilon) const; // [tested]

  /// \brief Inverts the rotation, so instead of rotating N degrees around an axis, the quaternion will rotate -N degrees around its axis.
  ///
  /// This modifies the quaternion in place. If you want to get the inverse as a copy, use the negation operator (-).
  void Invert();

  // *** Operators ***
public:
  /// \brief Returns a Quaternion that represents the negative / inverted rotation.
  const ezQuatTemplate operator-() const; // [tested]

  // *** Common Quaternion operations ***
public:
  /// \brief Returns the dot-product of the two quaternions (commutative, order does not matter).
  Type Dot(const ezQuatTemplate& rhs) const; // [tested]

  // *** Euler Angle Conversions ***
public:
  /// \brief Converts the quaternion to Euler angles
  void GetAsEulerAngles(ezAngle& out_x, ezAngle& out_y, ezAngle& out_z) const; // [tested]

  /// \brief Sets the quaternion from Euler angles
  static ezQuatTemplate<Type> MakeFromEulerAngles(const ezAngle& x, const ezAngle& y, const ezAngle& z); // [tested]
  /*[[deprecated("Use ezQuat::MakeFromEulerAngles() instead.")]]*/ void SetFromEulerAngles(const ezAngle& x, const ezAngle& y, const ezAngle& z) { *this = MakeFromEulerAngles(x, y, z); }
};

/// \brief Rotates v by q
template <typename Type>
const ezVec3Template<Type> operator*(const ezQuatTemplate<Type>& q, const ezVec3Template<Type>& v); // [tested]

/// \brief Concatenates the rotations of q1 and q2
template <typename Type>
const ezQuatTemplate<Type> operator*(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

template <typename Type>
bool operator==(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

template <typename Type>
bool operator!=(const ezQuatTemplate<Type>& q1, const ezQuatTemplate<Type>& q2); // [tested]

#include <Foundation/Math/Implementation/Quat_inl.h>
