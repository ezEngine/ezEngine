#pragma once

#include <Foundation/SimdMath/SimdQuat.h>

class EZ_FOUNDATION_DLL ezSimdTransform
{
public:
  EZ_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  ezSimdTransform(); // [tested]

  /// Sets position, rotation and scale.
  explicit ezSimdTransform(const ezSimdVec4f& vPosition, const ezSimdQuat& qRotation = ezSimdQuat::MakeIdentity(), const ezSimdVec4f& vScale = ezSimdVec4f(1.0f)); // [tested]

  /// Sets rotation.
  explicit ezSimdTransform(const ezSimdQuat& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static ezSimdTransform Make(const ezSimdVec4f& vPosition, const ezSimdQuat& qRotation = ezSimdQuat::MakeIdentity(), const ezSimdVec4f& vScale = ezSimdVec4f(1.0f)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static ezSimdTransform MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static ezSimdTransform MakeLocalTransform(const ezSimdTransform& globalTransformParent, const ezSimdTransform& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static ezSimdTransform MakeGlobalTransform(const ezSimdTransform& globalTransformParent, const ezSimdTransform& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  ezSimdFloat GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool HasMirrorScaling() const;

  /// Returns whether this transform has only uniform scaling (including scale == 1).
  bool HasOnlyUniformScaling() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const ezSimdTransform& rhs, const ezSimdFloat& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  ezSimdTransform GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  ezSimdMat4f GetAsMat4() const;                                            // [tested]

public:
  [[nodiscard]] ezSimdVec4f TransformPosition(const ezSimdVec4f& v) const;  // [tested]
  [[nodiscard]] ezSimdVec4f TransformDirection(const ezSimdVec4f& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const ezSimdTransform& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const ezSimdQuat& q);  // [tested]

  void operator+=(const ezSimdVec4f& v); // [tested]
  void operator-=(const ezSimdVec4f& v); // [tested]

public:
  ezSimdVec4f m_Position;
  ezSimdQuat m_Rotation;
  ezSimdVec4f m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
EZ_ALWAYS_INLINE const ezSimdVec4f operator*(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdQuat& q, const ezSimdTransform& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdTransform& t, const ezSimdQuat& q); // [tested]

/// Translates the ezSimdTransform by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransform operator+(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// Translates the ezSimdTransform by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransform operator-(const ezSimdTransform& t, const ezSimdVec4f& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdTransform& lhs, const ezSimdTransform& rhs); // [tested]

EZ_ALWAYS_INLINE bool operator==(const ezSimdTransform& t1, const ezSimdTransform& t2);                   // [tested]
EZ_ALWAYS_INLINE bool operator!=(const ezSimdTransform& t1, const ezSimdTransform& t2);                   // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransform_inl.h>
