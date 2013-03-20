#pragma once

#include <Foundation/Math/Vec3.h>

class EZ_FOUNDATION_DLL ezBoundingSphere
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

public:
  /// \brief Default constructor does not initialize any data.
  ezBoundingSphere();

  /// \brief Creates a sphere with the given radius around the given center.
  ezBoundingSphere(const ezVec3& vCenter, float fRadius); // [tested]

  /// \brief Sets all elements to Zero. The sphere is thus 'valid'.
  void SetZero(); // [tested]

  /// \brief Checks whether the sphere is all zero.
  bool IsZero(float fEpsilon = ezMath_DefaultEpsilon) const; // [tested]

  /// \brief Sets the bounding sphere to invalid values.
  void SetInvalid(); // [tested]

  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Sets the sphere to the given values.
  void SetElements(const ezVec3& vCenter, float fRadius); // [tested]

  /// \brief Initializes the sphere to be the bounding sphere of all the given points (not necessarily the smallest one).
  void SetFromPoints(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)); // [tested]

  /// \brief Increases the sphere's radius to include this point. Does NOT change its position, thus the resulting sphere might be not a very tight fit.
  void ExpandToInclude(const ezVec3& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere. Does not change the center position of this sphere.
  void ExpandToInclude(const ezBoundingSphere& rhs); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the box. Does not change the center position of this sphere.
  void ExpandToInclude(const ezBoundingBox& rhs); // [tested]

  /// \brief Increases the size of the sphere by the given amount.
  void Grow(float fDiff); // [tested]

  /// \brief Tests whether two spheres are identical.
  bool IsIdentical(const ezBoundingSphere& rhs) const; // [tested]

  /// \brief Tests whether two spheres are equal within some threshold.
  bool IsEqual(const ezBoundingSphere& rhs, float fEpsilon = ezMath_DefaultEpsilon) const; // [tested]

  /// \brief Moves the sphere by the given vector.
  void Translate(const ezVec3& vTranslation); // [tested]

  /// \brief Scales the sphere's size, does not change its center position.
  void ScaleFromCenter(float fScale); // [tested]

  /// \brief Scales the sphere in world unites, meaning its center position will change as well.
  void ScaleFromOrigin(const ezVec3& vScale); // [tested]

  /// \brief Transforms the sphere with the given matrix from the world origin. Ie. scalings and rotations will influence its position.
  void TransformFromOrigin(const ezMat4& mTransform); // [tested]

  /// \brief Transforms the sphere with the given matrix from its own center. Ie. rotations have no effect, scalings will only affect the radius, and only translations will affect its position.
  void TransformFromCenter(const ezMat4& mTransform); // [tested]

  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  float GetDistanceTo(const ezVec3& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for overlapping spheres.
  float GetDistanceTo(const ezBoundingSphere& rhs) const; // [tested]

  /// \brief Returns the minimum distance between the box and the sphere. Zero if both are exactly touching, negative values if they overlap.
  float GetDistanceTo(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Returns the minimum distance of any of the points to the sphere.
  float GetDistanceTo(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const ezVec3& vPoint) const; // [tested]

  /// \brief Returns whether all the given points are inside this sphere.
  bool Contains(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const ezBoundingSphere& rhs) const; // [tested]

  /// \brief Returns whether the given box is completely inside this sphere.
  bool Contains(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Checks whether any of the given points is inside the sphere
  bool Overlaps(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezVec3)) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const ezBoundingSphere& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const ezBoundingBox& rhs) const; // [tested]

  /// \brief Returns a bounding box that encloses this sphere.
  const ezBoundingBox GetBoundingBox() const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the closest distance to the original point.
  const ezVec3 GetClampedPoint(const ezVec3& vPoint); // [tested]

  /// \brief Computes the intersection of a ray with this sphere. Returns true if there was an intersection. May optionally return the intersection time and position. The ray's direction must be normalized.
  /// The function will also return true, if the ray already starts inside the sphere, but it will still compute the intersection with the surface of the sphere.
  bool GetRayIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, float* out_fIntersection = NULL, ezVec3* out_vIntersection = NULL) const; // [tested]

  /// \brief Returns true if the line segment intersects the spere.
  bool GetLineSegmentIntersection(const ezVec3& vLineStartPos, const ezVec3& vLineEndPos, float* out_fHitFraction = NULL, ezVec3* out_vIntersection = NULL) const; // [tested]


public:

  ezVec3 m_vCenter;
  float m_fRadius;
};

/// \brief Checks whether this sphere and the other are identical.
bool operator== (const ezBoundingSphere& lhs, const ezBoundingSphere& rhs); // [tested]

/// \brief Checks whether this sphere and the other are not identical.
bool operator!= (const ezBoundingSphere& lhs, const ezBoundingSphere& rhs); // [tested]


#include <Foundation/Math/Implementation/BoundingSphere_inl.h>

