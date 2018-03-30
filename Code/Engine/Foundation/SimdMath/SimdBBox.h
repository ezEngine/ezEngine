#pragma once

#include <Foundation/SimdMath/SimdBSphere.h>

class ezSimdBBox
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  ezSimdBBox();

  /// \brief Constructs the box with the given minimum and maximum values.
  ezSimdBBox(const ezSimdVec4f& vMin, const ezSimdVec4f& vMax); // [tested]

public:

  /// \brief Resets the box to an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  void SetInvalid(); // [tested]

  /// \brief Sets the box from a center point and half-extents for each axis.
  void SetCenterAndHalfExtents(const ezSimdVec4f& vCenter, const ezSimdVec4f& vHalfExtents); // [tested]

  /// \brief Creates a new bounding-box around the given set of points.
  void SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center position of the box.
  ezSimdVec4f GetCenter() const; // [tested]

  /// \brief Returns the extents of the box along each axis.
  ezSimdVec4f GetExtents() const; // [tested]

  /// \brief Returns the half extents of the box along each axis.
  ezSimdVec4f GetHalfExtents() const; // [tested]

  /// \brief Expands the box such that the given point is inside it.
  void ExpandToInclude(const ezSimdVec4f& vPoint); // [tested]

  /// \brief Expands the box such that all the given points are inside it.
  void ExpandToInclude(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Expands the box such that the given box is inside it.
  void ExpandToInclude(const ezSimdBBox& rhs); // [tested]

  /// \brief If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]


  /// \brief Checks whether the given point is inside the box.
  bool Contains(const ezSimdVec4f& vPoint) const; // [tested]

  /// \brief Checks whether the given box is completely inside this box.
  bool Contains(const ezSimdBBox& rhs) const; // [tested]

  /// \brief Checks whether the given sphere is completely inside this box.
  bool Contains(const ezSimdBSphere& sphere) const; // [tested]

  /// \brief Checks whether this box overlaps with the given box.
  bool Overlaps(const ezSimdBBox& rhs) const; // [tested]

  /// \brief Checks whether the given sphere overlaps with this box.
  bool Overlaps(const ezSimdBSphere& sphere) const; // [tested]


  /// \brief Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const ezSimdVec4f& vDiff); // [tested]

  /// \brief Moves the box by the given vector.
  void Translate(const ezSimdVec4f& vDiff); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const ezSimdTransform& transform); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const ezSimdMat4f& mat); // [tested]


  /// \brief The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest possible distance to the original point.
  ezSimdVec4f GetClampedPoint(const ezSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  ezSimdFloat GetDistanceSquaredTo(const ezSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  ezSimdFloat GetDistanceTo(const ezSimdVec4f& vPoint) const; // [tested]


  bool operator==(const ezSimdBBox& rhs) const; // [tested]
  bool operator!=(const ezSimdBBox& rhs) const; // [tested]

public:
  ezSimdVec4f m_Min;
  ezSimdVec4f m_Max;
};

#include <Foundation/SimdMath/Implementation/SimdBBox_inl.h>

