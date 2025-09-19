#pragma once

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// \brief Represents position, rotation and scaling using separate components for efficient hierarchical transformations.
///
/// Transform operations follow the order: Scale → Rotation → Translation (SRT).
/// This ensures that scale and rotation are applied in local space, while translation
/// occurs in global space. This makes the translation component represent the global
/// center position around which objects rotate.
///
/// Hierarchical transformation support:
/// - MakeLocalTransform(): Creates the local transformation difference between parent and child
/// - MakeGlobalTransform(): Combines parent's global transform with child's local transform
/// - Useful for editing entities in hierarchical structures where you need to factor in parent transforms
///
/// Limitations:
/// - Cannot represent shearing transformations
/// - Non-uniform scaling across hierarchies uses simplified component-wise multiplication
/// - When these limitations are problematic, use ezMat3 or ezMat4T instead
///
/// Performance characteristics:
/// - More memory efficient than 4x4 matrices (7 floats vs 16)
/// - Faster interpolation and concatenation than matrices
/// - Suitable for real-time animation and game object hierarchies
template <typename Type>
class ezTransformTemplate
{
public:
  EZ_DECLARE_POD_TYPE();

  // *** Data ***
  ezVec3Template<Type> m_vPosition;
  ezQuatTemplate<Type> m_qRotation;
  ezVec3Template<Type> m_vScale;

  // *** Constructors ***
public:
  /// \brief Default constructor: Does not do any initialization.
  ezTransformTemplate() = default;


  /// \brief Initializes the transform from the given position, rotation and scale.
  explicit ezTransformTemplate(const ezVec3Template<Type>& vPosition,
    const ezQuatTemplate<Type>& qRotation = ezQuatTemplate<Type>::MakeIdentity(),
    const ezVec3Template<Type>& vScale = ezVec3Template<Type>(1)); // [tested]

  /// \brief Creates a transform from the given position, rotation and scale.
  [[nodiscard]] static ezTransformTemplate<Type> Make(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation = ezQuatTemplate<Type>::MakeIdentity(), const ezVec3Template<Type>& vScale = ezVec3Template<Type>(1));

  /// \brief Creates an identity transform.
  [[nodiscard]] static ezTransformTemplate<Type> MakeIdentity();

  /// \brief Creates a transform from the given matrix using decomposition.
  ///
  /// Attempts to decompose the matrix into separate translation, rotation, and scale components.
  /// The operation always succeeds but may produce unexpected results with invalid matrices.
  ///
  /// \note Matrices containing shearing cannot be accurately represented as transforms.
  /// Mirroring (negative determinant) may not be preserved correctly.
  /// Zero or near-zero matrices will produce degenerate transforms.
  [[nodiscard]] static ezTransformTemplate<Type> MakeFromMat4(const ezMat4Template<Type>& mMat);

  /// \brief Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  ///
  /// Computes: localTransform = inverse(globalTransformParent) * globalTransformChild
  [[nodiscard]] static ezTransformTemplate<Type> MakeLocalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& globalTransformChild); // [tested]

  /// \brief Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  ///
  /// Computes: globalTransform = globalTransformParent * localTransformChild
  [[nodiscard]] static ezTransformTemplate<Type> MakeGlobalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& localTransformChild); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  Type GetMaxScale() const;

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  ///
  /// Checks if any scale component is negative, which indicates mirroring along that axis.
  /// Important for correct normal vector transformations and culling operations.
  bool HasMirrorScaling() const;

  /// \brief Returns whether this transform contains uniform scaling.
  ///
  /// Returns true if all three scale components have the same absolute value.
  bool ContainsUniformScale() const;

  /// \brief Checks that all components are valid (no NaN, only finite numbers).
  bool IsValid() const;

  // *** Equality ***
public:
  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezTransformTemplate& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezTransformTemplate& rhs, Type fEpsilon) const; // [tested]

  // *** Inverse ***
public:
  /// \brief Inverts this transform.
  void Invert(); // [tested]

  /// \brief Returns the inverse of this transform.
  const ezTransformTemplate GetInverse() const; // [tested]

  /// \brief Transforms a position vector by this transform (applies scale, rotation, and translation).
  [[nodiscard]] ezVec3Template<Type> TransformPosition(const ezVec3Template<Type>& v) const; // [tested]

  /// \brief Transforms a direction vector by this transform (applies scale and rotation, but not translation).
  [[nodiscard]] ezVec3Template<Type> TransformDirection(const ezVec3Template<Type>& v) const; // [tested]

  /// \brief Translates the transform by the given vector in global space.
  void operator+=(const ezVec3Template<Type>& v); // [tested]

  /// \brief Translates the transform by the negative of the given vector in global space.
  void operator-=(const ezVec3Template<Type>& v); // [tested]

  // *** Conversion operations ***
public:
  /// \brief Returns the transformation as a matrix.
  const ezMat4Template<Type> GetAsMat4() const; // [tested]
};

// *** free functions ***

/// \brief Transforms the vector v by the transform (equivalent to TransformPosition).
template <typename Type>
const ezVec3Template<Type> operator*(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the left with t.
template <typename Type>
const ezTransformTemplate<Type> operator*(const ezQuatTemplate<Type>& q, const ezTransformTemplate<Type>& t); // [tested]

/// \brief Rotates the transform by the given quaternion. Multiplies q from the right with t.
template <typename Type>
const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t, const ezQuatTemplate<Type>& q);

/// \brief Translates the ezTransform by the vector. This will move the object in global space.
template <typename Type>
const ezTransformTemplate<Type> operator+(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v); // [tested]

/// \brief Translates the ezTransform by the vector. This will move the object in global space.
template <typename Type>
const ezTransformTemplate<Type> operator-(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v); // [tested]

/// \brief Concatenates the two transforms. This is the same as a matrix multiplication, thus not commutative.
///
/// Computes: result = t1 * t2 (apply t2 first, then t1)
/// Equivalent to: MakeGlobalTransform(t1, t2)
template <typename Type>
const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2); // [tested]

template <typename Type>
bool operator==(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2);                           // [tested]

template <typename Type>
bool operator!=(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2);                           // [tested]

#include <Foundation/Math/Implementation/Transform_inl.h>
