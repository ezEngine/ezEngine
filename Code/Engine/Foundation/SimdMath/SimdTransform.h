#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class EZ_FOUNDATION_DLL ezSimdTransform
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  ezSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  ezSimdTransform(const ezSimdVec4f& position, const ezSimdQuat& rotation = ezSimdQuat::Identity(), const ezSimdVec4f& scale = ezSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  ezSimdTransform(const ezSimdQuat& rotation); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Transform.
  static ezSimdTransform Identity(); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  ezSimdFloat GetMaxScale() const; // [tested]

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

public:
  /// \brief Equality Check with epsilon
  bool IsEqual(const ezSimdTransform& rhs, const ezSimdFloat& fEpsilon) const; // [tested]

public:
  /// \brief Inverts this transform.
  void Invert(); // [tested]

  /// \brief Returns the inverse of this transform.
  ezSimdTransform GetInverse() const; // [tested]

public:
  /// \brief Sets this transform to be the local transformation needed to get from the parent's transform to the child's.
  void SetLocalTransform(const ezSimdTransform& GlobalTransformParent, const ezSimdTransform& GlobalTransformChild); // [tested]

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global one.
  void SetGlobalTransform(const ezSimdTransform& GlobalTransformParent, const ezSimdTransform& LocalTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  ezSimdMat4f GetAsMat4() const; // [tested]

public:

  ezSimdVec4f TransformPosition(const ezSimdVec4f& v) const; // [tested]
  ezSimdVec4f TransformDirection(const ezSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  ezSimdTransform operator*(const ezSimdTransform& other) const; // [tested]
  void operator*=(const ezSimdTransform& other); // [tested]

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  //ezSimdTransform operator*(const ezSimdQuat& q) const; // [tested]
  void operator*=(const ezSimdQuat& q); // [tested]

  /// \brief Translates the transform by the vector. This will move the object in global space.
  ezSimdTransform operator+(const ezSimdVec4f& v) const; // [tested]
  ezSimdTransform operator-(const ezSimdVec4f& v) const; // [tested]

  void operator+=(const ezSimdVec4f& v); // [tested]
  void operator-=(const ezSimdVec4f& v); // [tested]

  bool operator==(const ezSimdTransform& other) const; // [tested]
  bool operator!=(const ezSimdTransform& other) const; // [tested]

public:
  ezSimdVec4f m_Position;
  ezSimdQuat m_Rotation;
  ezSimdVec4f m_Scale;
};

#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
