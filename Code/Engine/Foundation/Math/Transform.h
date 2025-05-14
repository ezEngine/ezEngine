#pragma once

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Vec3.h>

/// \todo Fix docs and unit tests

/// \brief A class that represents position, rotation and scaling via a position vector, a quaternion and a scale vector.
///
/// Scale is applied first, then rotation and finally translation is added. Thus scale and rotation are always in 'local space',
/// i.e. applying a rotation to the ezTransformTemplate will rotate objects in place around their local center.
/// Since the translation is added afterwards, the translation component is always the global center position, around which
/// objects rotate.
///
/// The functions SetLocalTransform() and SetGlobalTransform() allow to create transforms that either represent the full
/// global transformation of an object, factoring its parent's transform in, or the local transformation that will get you
/// from the parent's global transformation to the current global transformation of a child (i.e. only the difference).
/// This is particularly useful when editing entities in a hierarchical structure.
///
/// This representation cannot handle shearing, which means rotations and scalings cannot be combined correctly.
/// Many parts of game engine cannot handle shearing or non-uniform scaling across hierarchies anyway. Therefore this
/// class implements a simplified way of combining scalings when multiplying two ezTransform's. Instead of rotating scale into
/// the proper space, the two values are simply multiplied component-wise.
///
/// In situations where this is insufficient, use a 3x3 or 4x4 matrix instead. Sometimes it is sufficient to use the matrix for
/// the computation and the result can be stored in a transform again.
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

  /// \brief Creates a transform from the given matrix.
  ///
  /// \note This operation always succeeds, even though the matrix may be complete garbage (e.g. a zero matrix)
  /// or may not be representable as a transform (containing shearing).
  /// Also be careful with mirroring. The transform may or may not be able to represent that.
  [[nodiscard]] static ezTransformTemplate<Type> MakeFromMat4(const ezMat4Template<Type>& mMat);

  /// \brief Creates a transform that is the local transformation needed to get from the parent's transform to the child's.
  [[nodiscard]] static ezTransformTemplate<Type> MakeLocalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& globalTransformChild); // [tested]

  /// \brief Creates a transform that is the global transform, that is reached by applying the child's local transform to the parent's global one.
  [[nodiscard]] static ezTransformTemplate<Type> MakeGlobalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& localTransformChild); // [tested]

  /// \brief Sets the position to be zero and the rotation to identity.
  void SetIdentity(); // [tested]

  /// \brief Returns the scale component with maximum magnitude.
  Type GetMaxScale() const;

  /// \brief Returns whether this transform contains negative scaling aka mirroring.
  bool HasMirrorScaling() const;

  /// \brief Returns whether this transform contains uniform scaling.
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
  const ezTransformTemplate GetInverse() const;                                               // [tested]

  [[nodiscard]] ezVec3Template<Type> TransformPosition(const ezVec3Template<Type>& v) const;  // [tested]
  [[nodiscard]] ezVec3Template<Type> TransformDirection(const ezVec3Template<Type>& v) const; // [tested]

  void operator+=(const ezVec3Template<Type>& v);                                             // [tested]
  void operator-=(const ezVec3Template<Type>& v);                                             // [tested]

  // *** Conversion operations ***
public:
  /// \brief Returns the transformation as a matrix.
  const ezMat4Template<Type> GetAsMat4() const; // [tested]
};

// *** free functions ***

/// \brief Transforms the vector v by the transform.
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
template <typename Type>
const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2); // [tested]

template <typename Type>
bool operator==(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2);                           // [tested]

template <typename Type>
bool operator!=(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2);                           // [tested]

#include <Foundation/Math/Implementation/Transform_inl.h>
