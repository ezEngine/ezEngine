#pragma once

#include <Foundation/SimdMath/SimdTransform.h>

class EZ_FOUNDATION_DLL ezSimdBSphere
{
public:
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize any data.
  ezSimdBSphere();

  /// \brief Creates a sphere with the given radius around the given center.
  ezSimdBSphere(const ezSimdVec4f& vCenter, const ezSimdFloat& fRadius); // [tested]

public:
  /// \brief Sets the bounding sphere to invalid values.
  void SetInvalid(); // [tested]

  /// \brief Returns whether the sphere has valid values.
  bool IsValid() const; // [tested]

  /// \brief Returns whether any value is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center
  ezSimdVec4f GetCenter() const; // [tested]

  /// \brief Returns the radius
  ezSimdFloat GetRadius() const; // [tested]

  /// \brief Initializes the sphere to be the bounding sphere of all the given points.
  void SetFromPoints(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Increases the sphere's radius to include this point.
  void ExpandToInclude(const ezSimdVec4f& vPoint); // [tested]

  /// \brief Increases the sphere's radius to include all given points. Does NOT change its position, thus the resulting sphere might be not a very tight fit. More efficient than calling this for every point individually.
  void ExpandToInclude(const ezSimdVec4f* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride = sizeof(ezSimdVec4f)); // [tested]

  /// \brief Increases this sphere's radius, such that it encloses the other sphere.
  void ExpandToInclude(const ezSimdBSphere& rhs); // [tested]

public:
  /// \brief Transforms the sphere in its local space.
  void Transform(const ezSimdTransform& t); // [tested]

public:
  /// \brief Computes the distance of the point to the sphere's surface. Returns negative values for points inside the sphere.
  ezSimdFloat GetDistanceTo(const ezSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the distance between the two spheres. Zero for spheres that are exactly touching each other, negative values for overlapping spheres.
  ezSimdFloat GetDistanceTo(const ezSimdBSphere& rhs) const; // [tested]

  /// \brief Returns true if the given point is inside the sphere.
  bool Contains(const ezSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns whether the other sphere is completely inside this sphere.
  bool Contains(const ezSimdBSphere& rhs) const; // [tested]

  /// \brief Checks whether the two objects overlap.
  bool Overlaps(const ezSimdBSphere& rhs) const; // [tested]

  /// \brief Clamps the given position to the volume of the sphere. The resulting point will always be inside the sphere, but have the closest distance to the original point.
  ezSimdVec4f GetClampedPoint(const ezSimdVec4f& vPoint); // [tested]

  bool operator==(const ezSimdBSphere& rhs) const; // [tested]
  bool operator!=(const ezSimdBSphere& rhs) const; // [tested]

public:
  ezSimdVec4f m_CenterAndRadius;
};

#include <Foundation/SimdMath/Implementation/SimdBSphere_inl.h>
