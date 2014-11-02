#pragma once

#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>

/// \brief A class that represents a complete position, rotation and scaling through an ezVec3 and an ezMat3.
///
/// The rotation is applied first and then the translation is added. Thus the rotation component is always in 'local space',
/// i.e. applying a rotation to the ezTransform will rotate objects in place around their local center.
/// Since the translation is added afterwards, the translation component is always the global center position, around which
/// objects also rotate.
///
/// The functions SetLocalTransform() and SetGlobalTransform() allow to create transforms that either represent the full
/// global transformation of an object, factoring its parent's transform in, or the local transformation that will get you
/// from the parent's global transformation to the current global transformation of a child (i.e. only the difference).
/// This is particularly useful when editing entities in a hierarchical structure.
///
/// Since the 'm_Rotation' member is a full ezMat3, it may also contain scaling, including non-uniform scaling and shearing.
class EZ_FOUNDATION_DLL ezTransform
{
  // *** Data ***
public:

  ezMat3 m_Rotation;
  ezVec3 m_vPosition;

  /// \brief Copies the 12 values of this matrix into the given array. 'layout' defines whether the data should end up in column-major or row-major format.
  void GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const; // [tested]

  // *** Constructors ***
public:

  /// \brief Default constructor: Does not do any initialization.
  ezTransform() { } // [tested]

  /// \brief Sets position and rotation matrix, which might include scale as well.
  explicit ezTransform(const ezVec3& vPosition, const ezMat3& Rotation); // [tested]

  /// \brief Sets position and rotation.
  explicit ezTransform(const ezVec3& vPosition, const ezQuat& qRotation = ezQuat::IdentityQuaternion()); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit ezTransform(const ezVec3& vPosition, const ezQuat& qRotation, const ezVec3& vScale); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  // *** Equality ***
public:

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezTransform& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezTransform& rhs, float fEpsilon) const; // [tested]

  // *** Inverse ***
public:

  /// \brief Inverts this transform. Return value indicates whether it could be Inverted.
  ezResult Invert(float fEpsilon = ezMath::BasicType<float>::SmallEpsilon()); // [tested]

  /// \brief Returns the inverse of this transform.
  const ezTransform GetInverse() const; // [tested]

  // *** Conversion operations ***
public:

  /// \brief Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
  void SetLocalTransform(const ezTransform& GlobalTransformParent, const ezTransform& GlobalTransformChild); // [tested]

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global one.
  void SetGlobalTransform(const ezTransform& GlobalTransformParent, const ezTransform& LocalTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  const ezMat4 GetAsMat4() const; // [tested]


  // *** Operators ***
public:

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const ezQuat& q); // [tested]

};


// *** free functions ***

/// \brief Transforms the vector v by the transform.
const ezVec3 operator*(const ezTransform& t, const ezVec3& v); // [tested]

/// \brief Rotates the transform by the given quaternion.
const ezTransform operator*(const ezQuat& q, const ezTransform& t); // [tested]

/// \brief Translates the ezTransform by the vector. This will move the object in global space.
const ezTransform operator+(const ezTransform& t, const ezVec3& v); // [tested]

/// \brief Translates the ezTransform by the vector. This will move the object in global space.
const ezTransform operator-(const ezTransform& t, const ezVec3& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
const ezTransform operator*(const ezTransform& t1, const ezTransform& t2); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
const ezTransform operator*(const ezTransform& t1, const ezMat4& t2); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
const ezTransform operator*(const ezMat4& t1, const ezTransform& t2); // [tested]

#include <Foundation/Math/Implementation/Transform_inl.h>



