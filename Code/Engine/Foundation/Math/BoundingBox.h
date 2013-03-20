#pragma once

#include <Foundation/Math/Vec3.h>

class EZ_FOUNDATION_DLL ezBoundingBox
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

public:
  /// \brief Default constructor does not initialize anything.
  ezBoundingBox();

  /// \brief Constructs the box with the given minimum and maximum values.
  ezBoundingBox(const ezVec3& vMin, const ezVec3& vMax); // [tested]

  /// \brief Resets the box to an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  void SetInvalid(); // [tested]

  /// \brief Sets the box from a center point and half-extents for each axis.
  void SetCenterAndHalfExtents(const ezVec3& vCenter, const ezVec3& vHalfExtents); // [tested]

  /// \brief Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Directly sets the minimum and maximum values.
  void SetElements(const ezVec3& vMin, const ezVec3& vMax); // [tested]

  /// \brief Creates a new bounding-box around the given set of points.
  void SetFromPoints(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)); // [tested]

  /// \brief Writes the 8 different corners of the box to the given array.
  void GetCorners(ezVec3* out_pCorners) const; // [tested]

  /// \brief Returns the center position of the box.
  const ezVec3 GetCenter() const; // [tested]

  /// \brief Returns the extents of the box along each axis.
  const ezVec3 GetExtents() const; // [tested]

  /// \brief Returns the half extents of the box along each axis.
  const ezVec3 GetHalfExtents() const; // [tested]

  /// \brief Expands the box such that the given point is inside it.
  void ExpandToInclude(const ezVec3& vPoint); // [tested]

  /// \brief Expands the box such that the given box is inside it.
  void ExpandToInclude(const ezBoundingBox& rhs); // [tested]

  /// \brief Expands the box such that all the given points are inside it.
  void ExpandToInclude(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)); // [tested]

  /// \brief If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]

  /// \brief Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const ezVec3& vDiff); // [tested]

  /// \brief Checks whether the given point is inside the box.
  bool Contains(const ezVec3& vPoint) const; // [tested]

  /// \brief Checks whether the given box is completely inside this box.
  bool Contains(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Checks whether all the given points are inside this box.
  bool Contains(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)) const; // [tested]

  /// \brief Checks whether the given sphere is completely inside this box.
  bool Contains(const ezBoundingSphere& sphere) const; // [tested]

  /// \brief Checks whether this box overlaps with the given box.
  bool Overlaps(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Checks whether any of the given points is inside this box.
  bool Overlaps(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)) const; // [tested]

  /// \brief Checks whether the given sphere overlaps with this box.
  bool Overlaps(const ezBoundingSphere& sphere) const; // [tested]

  /// \brief Checks whether this box and the other box are exactly identical.
  bool IsIdentical(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Checks whether this box and the other box are equal within some threshold.
  bool IsEqual(const ezBoundingBox& rhs, float fEpsilon = ezMath_DefaultEpsilon) const; // [tested]

  /// \brief Moves the box by the given vector.
  void Translate(const ezVec3& vDiff); // [tested]

  /// \brief Scales the box along each axis, but keeps its center constant.
  void ScaleFromCenter(const ezVec3& vScale); // [tested]

  /// \brief Scales the box's corners by the given factors, thus also moves the box around.
  void ScaleFromOrigin(const ezVec3& vScale); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points. Rotations and scalings will influence the center position of the box.
  void TransformFromCenter(const ezMat4& mTransform); // [tested]

  /// \brief Transforms the corners of the box in its local space. The center of the box does not change, unless the transform contains a translation.
  void TransformFromOrigin(const ezMat4& mTransform); // [tested]

  /// \brief The given point is clamped to the volume of the box, ie. it will be either inside the box or on its surface and it will have the closest possible distance to the original point.
  const ezVec3 GetClampedPoint(const ezVec3& vPoint) const; // [tested]

  /// \brief Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  float GetDistanceSquaredTo(const ezVec3& vPoint) const; // [tested]

  /// \brief Returns the minimum squared distance between the two boxes. Zero if the boxes overlap.
  float GetDistanceSquaredTo(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  float GetDistanceTo(const ezVec3& vPoint) const; // [tested]

  /// \brief Returns the minimum distance between the box and the sphere. Zero or negative if both overlap.
  float GetDistanceTo(const ezBoundingSphere& sphere) const; // [tested]

  /// \brief Returns the minimum distance between the two boxes. Zero if the boxes overlap.
  float GetDistanceTo(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Returns whether the given ray intersects the box. Optionally returns the intersection distance and position.
  bool GetRayIntersection(const ezVec3& vStartPos, const ezVec3& vRayDirNormalized, float* out_fIntersection = NULL, ezVec3* out_vIntersection = NULL) const; // [tested]

  /// \brief Checks whether the line segment intersects the box. Optionally returns the intersection point and the fraction along the line segment where the intersection occurred.
  bool GetLineSegmentIntersection(const ezVec3& vStartPos, const ezVec3& vEndPos, float* out_fLineFraction = NULL, ezVec3* out_vIntersection = NULL) const; // [tested]

  /// \brief Returns a bounding sphere that encloses this box.
  const ezBoundingSphere GetBoundingSphere() const; // [tested]


public:

  ezVec3 m_vMin;
  ezVec3 m_vMax;
};

/// \brief Checks whether this box and the other are identical.
bool operator== (const ezBoundingBox& lhs, const ezBoundingBox& rhs); // [tested]

/// \brief Checks whether this box and the other are not identical.
bool operator!= (const ezBoundingBox& lhs, const ezBoundingBox& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingBox_inl.h>

