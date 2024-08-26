#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class EZ_FOUNDATION_DLL ezSimdBBoxSphere
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  ezSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  [[deprecated("Use MakeFromCenterExtents() instead.")]] ezSimdBBoxSphere(const ezSimdVec4f& vCenter, const ezSimdVec4f& vBoxHalfExtents, const ezSimdFloat& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  [[deprecated("Use MakeFromBoxAndSphere() instead.")]] ezSimdBBoxSphere(const ezSimdBBox& box, const ezSimdBSphere& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  ezSimdBBoxSphere(const ezSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  ezSimdBBoxSphere(const ezSimdBSphere& sphere); // [tested]

  /// \brief Creates an object with all zero values. These are valid bounds around the origin with no volume.
  [[nodiscard]] static ezSimdBBoxSphere MakeZero();

  /// \brief Creates an 'invalid' object, ie one with negative extents/radius. Invalid objects can be made valid through ExpandToInclude().
  [[nodiscard]] static ezSimdBBoxSphere MakeInvalid(); // [tested]

  /// \brief Creates an object from the given center point and extents.
  [[nodiscard]] static ezSimdBBoxSphere MakeFromCenterExtents(const ezSimdVec4f& vCenter, const ezSimdVec4f& vBoxHalfExtents, const ezSimdFloat& fSphereRadius);

  /// \brief Creates an object that contains all the provided points.
  [[nodiscard]] static ezSimdBBoxSphere MakeFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f));

  /// \brief Creates an object from another bounding box.
  [[nodiscard]] static ezSimdBBoxSphere MakeFromBox(const ezSimdBBox& box);

  /// \brief Creates an object from another bounding sphere.
  [[nodiscard]] static ezSimdBBoxSphere MakeFromSphere(const ezSimdBSphere& sphere);

  /// \brief Creates an object from another bounding box and a sphere.
  [[nodiscard]] static ezSimdBBoxSphere MakeFromBoxAndSphere(const ezSimdBBox& box, const ezSimdBSphere& sphere);


public:
  /// \brief Resets the bounds to an invalid state.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Returns the bounding box.
  ezSimdBBox GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  ezSimdBSphere GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const ezSimdBBoxSphere& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const ezSimdTransform& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const ezSimdMat4f& mMat);                          // [tested]

  [[nodiscard]] bool operator==(const ezSimdBBoxSphere& rhs) const; // [tested]
  [[nodiscard]] bool operator!=(const ezSimdBBoxSphere& rhs) const; // [tested]

public:
  ezSimdVec4f m_CenterAndRadius;
  ezSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>
