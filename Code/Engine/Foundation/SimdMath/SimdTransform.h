#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class EZ_FOUNDATION_DLL ezSimdTransform
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor: Does not do any initialization.
  ezSimdTransform(); // [tested]

  /// \brief Sets position, rotation and scale.
  explicit ezSimdTransform(const ezSimdVec4f& position, const ezSimdQuat& rotation = ezSimdQuat::IdentityQuaternion(),
    const ezSimdVec4f& scale = ezSimdVec4f(1.0f)); // [tested]

  /// \brief Sets rotation.
  explicit ezSimdTransform(const ezSimdQuat& rotation); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns an Identity Transform.
  static ezSimdTransform IdentityTransform(); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  ezSimdFloat GetMaxScale() const; // [tested]

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool ContainsNegativeScale() const;

  /// \brief Returns whether this transform contains uniform scaling.
  bool ContainsUniformScale() const;

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

  /// \brief Sets this transform to the global transform, that is reached by applying the child's local transform to the parent's global
  /// one.
  void SetGlobalTransform(const ezSimdTransform& GlobalTransformParent, const ezSimdTransform& LocalTransformChild); // [tested]

  /// \brief Returns the transformation as a matrix.
  ezSimdMat4f GetAsMat4() const; // [tested]

public:
  ezSimdVec4f TransformPosition(const ezSimdVec4f& v) const;  // [tested]
  ezSimdVec4f TransformDirection(const ezSimdVec4f& v) const; // [tested]

  /// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const ezSimdTransform& other); // [tested]

  /// \brief Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const ezSimdQuat& q); // [tested]

  void operator+=(const ezSimdVec4f& v); // [tested]
  void operator-=(const ezSimdVec4f& v); // [tested]

public:
  ezSimdVec4f m_Position;
  ezSimdQuat m_Rotation;
  ezSimdVec4f m_Scale;
};

// *** free functions ***

/// \brief Transforms the vector v by the transform.
EZ_ALWAYS_INLINE const ezSimdVec4f operator*(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdQuat& q, const ezSimdTransform& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdTransform& t, const ezSimdQuat& q); // [tested]

/// \brief Translates the ezSimdTransform by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransform operator+(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// \brief Translates the ezSimdTransform by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransform operator-(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdTransform& lhs, const ezSimdTransform& rhs); // [tested]

EZ_ALWAYS_INLINE bool operator==(const ezSimdTransform& t1, const ezSimdTransform& t2); // [tested]
EZ_ALWAYS_INLINE bool operator!=(const ezSimdTransform& t1, const ezSimdTransform& t2); // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
