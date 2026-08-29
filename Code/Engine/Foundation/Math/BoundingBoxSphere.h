#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Vec4.h>

/// A combination of a bounding box and a bounding sphere with the same center.
///
/// This class uses less memory than storying a bounding box and sphere separate.

template <typename Type>
class ezBoundingBoxSphereTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  using ComponentType = Type;

public:
  /// Default constructor does not initialize anything.
  ezBoundingBoxSphereTemplate(); // [tested]

  ezBoundingBoxSphereTemplate(const ezBoundingBoxSphereTemplate& rhs);

  void operator=(const ezBoundingBoxSphereTemplate& rhs);

  /// Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  ezBoundingBoxSphereTemplate(const ezBoundingBoxTemplate<Type>& box); // [tested]

  /// Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  ezBoundingBoxSphereTemplate(const ezBoundingSphereTemplate<Type>& sphere); // [tested]

  /// Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeZero();

  /// Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeInvalid();

  /// Creates an object from the given center point and extents.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeFromCenterExtents(const ezVec3Template<Type>& vCenter, const ezVec3Template<Type>& vBoxHalfExtents, Type fSphereRadius);

  /// Creates an object that contains all the provided points.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeFromPoints(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3Template<Type>));

  /// Creates an object from another bounding box.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeFromBox(const ezBoundingBoxTemplate<Type>& box);

  /// Creates an object from another bounding sphere.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeFromSphere(const ezBoundingSphereTemplate<Type>& sphere);

  /// Creates an object from another bounding box and a sphere.
  [[nodiscard]] static ezBoundingBoxSphereTemplate<Type> MakeFromBoxAndSphere(const ezBoundingBoxTemplate<Type>& box, const ezBoundingSphereTemplate<Type>& sphere);


#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// Returns the bounding box.
  const ezBoundingBoxTemplate<Type> GetBox() const; // [tested]

  /// Returns the bounding sphere.
  const ezBoundingSphereTemplate<Type> GetSphere() const; // [tested]

  /// Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const ezBoundingBoxSphereTemplate& rhs); // [tested]

  /// Transforms the bounds in its local space.
  void Transform(const ezMat4Template<Type>& mTransform); // [tested]

public:
  ezVec3Template<Type> m_vCenter;
  Type m_fSphereRadius;
  ezVec3Template<Type> m_vBoxHalfExtents;
};

/// Checks whether this bounds and the other are identical.
template <typename Type>
bool operator==(const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs); // [tested]

/// Checks whether this bounds and the other are not identical.
template <typename Type>
bool operator!=(const ezBoundingBoxSphereTemplate<Type>& lhs, const ezBoundingBoxSphereTemplate<Type>& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBoxSphere_inl.h>
