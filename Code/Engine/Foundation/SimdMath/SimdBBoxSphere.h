#pragma once

#include <Foundation/SimdMath/SimdBBox.h>

class EZ_FOUNDATION_DLL ezSimdBBoxSphere
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  ezSimdBBoxSphere(); // [tested]

  /// \brief Constructs the bounds from the center position, the box half extends and the sphere radius.
  ezSimdBBoxSphere(const ezSimdVec4f& vCenter, const ezSimdVec4f& vBoxHalfExtents, const ezSimdFloat& fSphereRadius); // [tested]

  /// \brief Constructs the bounds from the given box and sphere.
  ezSimdBBoxSphere(const ezSimdBBox& box, const ezSimdBSphere& sphere); // [tested]

  /// \brief Constructs the bounds from the given box. The sphere radius is calculated from the box extends.
  ezSimdBBoxSphere(const ezSimdBBox& box); // [tested]

  /// \brief Constructs the bounds from the given sphere. The box extends are calculated from the sphere radius.
  ezSimdBBoxSphere(const ezSimdBSphere& sphere); // [tested]

public:
  /// \brief Resets the bounds to an invalid state.
  void SetInvalid(); // [tested]

  /// \brief Checks whether the bounds is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Calculates the bounds from given set of points.
  void SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Returns the bounding box.
  ezSimdBBox GetBox() const; // [tested]

  /// \brief Returns the bounding sphere.
  ezSimdBSphere GetSphere() const; // [tested]

  /// \brief Expands the bounds such that the given bounds are inside it.
  void ExpandToInclude(const ezSimdBBoxSphere& rhs); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const ezSimdTransform& t); // [tested]

  /// \brief Transforms the bounds in its local space.
  void Transform(const ezSimdMat4f& mat); // [tested]


  bool operator==(const ezSimdBBoxSphere& rhs) const; // [tested]
  bool operator!=(const ezSimdBBoxSphere& rhs) const; // [tested]

public:
  ezSimdVec4f m_CenterAndRadius;
  ezSimdVec4f m_BoxHalfExtents;
};

#include <Foundation/SimdMath/Implementation/SimdBBoxSphere_inl.h>

