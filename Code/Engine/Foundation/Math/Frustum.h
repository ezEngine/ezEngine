#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdBSphere.h>
#include <Foundation/SimdMath/SimdVec4b.h>
#include <Foundation/SimdMath/SimdVec4f.h>

/// \brief Enum that describes where in a volume another object is located.
struct ezVolumePosition
{
  /// \brief Enum that describes where in a volume another object is located.
  enum Enum
  {
    Outside,      //< means an object is ENTIRELY inside a volume
    Inside,       //< means an object is outside a volume
    Intersecting, //< means an object is PARTIALLY inside/outside a volume
  };
};

/// \brief Represents the frustum of some camera and can be used for culling objects.
///
/// The frustum always consists of exactly 6 planes (near, far, left, right, top, bottom).
///
/// The frustum planes point outwards, ie. when an object is in front of one of the planes, it is considered to be outside
/// the frustum.
///
/// Planes can be automatically extracted from a projection matrix or passed in manually.
/// In the latter case, make sure to pass them in in the order defined in the PlaneType enum.
class EZ_FOUNDATION_DLL ezFrustum
{
public:
  enum PlaneType
  {
    NearPlane,
    LeftPlane,
    RightPlane,
    FarPlane,
    BottomPlane,
    TopPlane,

    PLANE_COUNT
  };

  enum FrustumCorner
  {
    NearTopLeft,
    NearTopRight,
    NearBottomLeft,
    NearBottomRight,
    FarTopLeft,
    FarTopRight,
    FarBottomLeft,
    FarBottomRight,

    CORNER_COUNT = 8
  };

  /// \brief The constructor does NOT initialize the frustum planes, make sure to call SetFrustum() before trying to use it.
  ezFrustum();
  ~ezFrustum();

  /// \brief Sets the frustum manually by specifying the planes directly.
  ///
  /// \note Make sure to pass in the planes in the order of the PlaneType enum, otherwise ezFrustum may not always work as expected.
  void SetFrustum(const ezPlane* pPlanes); // [tested]

  /// \brief Creates the frustum by extracting the planes from the given (model-view / projection) matrix.
  ///
  /// If the matrix is just the projection matrix, the frustum will be in local space. Pass the full ModelViewProjection
  /// matrix to create the frustum in world-space.
  void SetFrustum(const ezMat4& ModelViewProjection, ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default, ezHandedness::Enum Handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a frustum from the given camera position, direction vectors and the field-of-view along X and Y.
  ///
  /// The up vector does not need to be exactly orthogonal to the forwards vector, it will get recomputed properly.
  /// FOV X and Y define the entire field-of-view, so a FOV of 180 degree would mean the entire half-space in front of the camera.
  void SetFrustum(const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle FovX, ezAngle FovY, float fNearPlane, float fFarPlane); // [tested]

  /// \brief Returns the n-th plane of the frustum.
  const ezPlane& GetPlane(ezUInt8 uiPlane) const; // [tested]

  /// \brief Returns the n-th plane of the frustum and allows modification.
  ezPlane& AccessPlane(ezUInt8 uiPlane);

  /// \brief Transforms the frustum by the given matrix. This allows to adjust the frustum to a new orientation when a camera is moved or
  /// when it is necessary to cull from a different position.
  void TransformFrustum(const ezMat4& mTransform); // [tested]

  /// \brief Flips all frustum planes around. Might be necessary after creating the frustum from a mirror projection matrix.
  void InvertFrustum(); // [tested]

  /// \brief Computes the frustum corner points.
  void ComputeCornerPoints(ezVec3 out_Points[FrustumCorner::CORNER_COUNT]) const; // [tested]

  /// \brief Checks whether the given object is inside or outside the frustum.
  ///
  /// A concave object might be classified as 'intersecting' although it is outside the frustum, if it overlaps the planes just right.
  /// However an object that overlaps the frustum is definitely never classified as 'outside'.
  ezVolumePosition::Enum GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices) const; // [tested]

  /// \brief Same as GetObjectPosition(), but applies a transformation to the given object first. This allows to do culling on instanced
  /// objects.
  ezVolumePosition::Enum GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices, const ezMat4& mObjectTransform) const; // [tested]

  /// \brief Checks whether the given object is inside or outside the frustum.
  ezVolumePosition::Enum GetObjectPosition(const ezBoundingSphere& Sphere) const; // [tested]

  /// \brief Checks whether the given object is inside or outside the frustum.
  ezVolumePosition::Enum GetObjectPosition(const ezBoundingBox& Box) const; // [tested]

  /// \brief Returns true if the object is fully inside the frustum or partially overlaps it. Returns false when the object is fully outside
  /// the frustum.
  ///
  /// This function is more efficient than GetObjectPosition() and should be preferred when possible.
  bool Overlaps(const ezSimdBBox& object) const; // [tested]

  /// \brief Returns true if the object is fully inside the frustum or partially overlaps it. Returns false when the object is fully outside
  /// the frustum.
  ///
  /// This function is more efficient than GetObjectPosition() and should be preferred when possible.
  bool Overlaps(const ezSimdBSphere& object) const; // [tested]

private:
  ezPlane m_Planes[PLANE_COUNT];
};

#include <Foundation/Math/Implementation/Frustum_inl.h>
