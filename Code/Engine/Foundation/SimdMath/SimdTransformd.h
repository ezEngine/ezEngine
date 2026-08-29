#pragma once

#include <Foundation/SimdMath/SimdQuatd.h>

class EZ_FOUNDATION_DLL ezSimdTransformd
{
public:
  EZ_DECLARE_POD_TYPE();

  /// Default constructor: Does not do any initialization.
  ezSimdTransformd(); // [tested]

  /// Sets position, rotation and scale.
  explicit ezSimdTransformd(const ezSimdVec4d& vPosition, const ezSimdQuatd& qRotation = ezSimdQuatd::MakeIdentity(), const ezSimdVec4d& vScale = ezSimdVec4d(1.0f)); // [tested]

  /// Sets rotation.
  explicit ezSimdTransformd(const ezSimdQuatd& qRotation); // [tested]

  /// Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static ezSimdTransformd Make(const ezSimdVec4d& vPosition, const ezSimdQuatd& qRotation = ezSimdQuatd::MakeIdentity(), const ezSimdVec4d& vScale = ezSimdVec4d(1.0f)); // [tested]

  /// Creates an identity transform.
  [[nodiscard]] static ezSimdTransformd MakeIdentity(); // [tested]

  /// Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static ezSimdTransformd MakeLocalTransform(const ezSimdTransformd& globalTransformParent, const ezSimdTransformd& globalTransformChild); // [tested]

  /// Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static ezSimdTransformd MakeGlobalTransform(const ezSimdTransformd& globalTransformParent, const ezSimdTransformd& localTransformChild); // [tested]

  /// Returns the scale component with maximum magnitude.
  ezSimdDouble GetMaxScale() const; // [tested]

  /// Returns whether this transform contains negative scaling aka mirroring.
  bool HasMirrorScaling() const;

  /// Returns whether this transform has only uniform scaling (including scale == 1).
  bool HasOnlyUniformScaling() const;

public:
  /// Equality Check with epsilon
  bool IsEqual(const ezSimdTransformd& rhs, const ezSimdDouble& fEpsilon) const; // [tested]

public:
  /// Inverts this transform.
  void Invert(); // [tested]

  /// Returns the inverse of this transform.
  ezSimdTransformd GetInverse() const; // [tested]

  /// Returns the transformation as a matrix.
  ezSimdMat4d GetAsMat4() const;                                            // [tested]

public:
  [[nodiscard]] ezSimdVec4d TransformPosition(const ezSimdVec4d& v) const;  // [tested]
  [[nodiscard]] ezSimdVec4d TransformDirection(const ezSimdVec4d& v) const; // [tested]

  /// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
  void operator*=(const ezSimdTransformd& other); // [tested]

  /// Multiplies \a q into the rotation component, thus rotating the entire transformation.
  void operator*=(const ezSimdQuatd& q);  // [tested]

  void operator+=(const ezSimdVec4d& v); // [tested]
  void operator-=(const ezSimdVec4d& v); // [tested]

public:
  ezSimdVec4d m_Position;
  ezSimdQuatd m_Rotation;
  ezSimdVec4d m_Scale;
};

// *** free functions ***

/// Transforms the vector v by the transform.
EZ_ALWAYS_INLINE const ezSimdVec4d operator*(const ezSimdTransformd& t, const ezSimdVec4d& v); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the left with t.
EZ_ALWAYS_INLINE const ezSimdTransformd operator*(const ezSimdQuatd& q, const ezSimdTransformd& t); // [tested]

/// Rotates the transform by the given quaternion. Multiplies q from the right with t.
EZ_ALWAYS_INLINE const ezSimdTransformd operator*(const ezSimdTransformd& t, const ezSimdQuatd& q); // [tested]

/// Translates the ezSimdTransformd by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransformd operator+(const ezSimdTransformd& t, const ezSimdVec4d& v); // [tested]

/// Translates the ezSimdTransformd by the vector. This will move the object in global space.
EZ_ALWAYS_INLINE const ezSimdTransformd operator-(const ezSimdTransformd& t, const ezSimdVec4d& v); // [tested]

/// Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
EZ_ALWAYS_INLINE const ezSimdTransformd operator*(const ezSimdTransformd& lhs, const ezSimdTransformd& rhs); // [tested]

EZ_ALWAYS_INLINE bool operator==(const ezSimdTransformd& t1, const ezSimdTransformd& t2);                   // [tested]
EZ_ALWAYS_INLINE bool operator!=(const ezSimdTransformd& t1, const ezSimdTransformd& t2);                   // [tested]


#include <Foundation/SimdMath/Implementation/SimdTransformd_inl.h>
