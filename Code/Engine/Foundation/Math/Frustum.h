#pragma once

#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Mat4.h>

/// \brief Enum that describes where in a volume another object is located.
struct ezVolumePosition
{
  /// \brief Enum that describes where in a volume another object is located.
  enum Enum
  {
    Outside,		  //< means an object is ENTIRELY inside a volume
    Inside,		    //< means an object is outside a volume
    Intersecting,	//< means an object is PARTIALLY inside/outside a volume
  };
};

/// \brief Represents the frustum of some camera and can be used for culling objects.
class EZ_FOUNDATION_DLL ezFrustum
{
public:
  /// \brief By default the frustum is empty.
  ezFrustum(); // [tested]

  /// \brief Sets the frustum manually by specifying the position and all planes at once.
  void SetFrustum(const ezVec3& vPosition, ezUInt8 uiNumPlanes, const ezPlane* pPlanes); // [tested]

  /// \brief Creates the frustum by extracting the planes from the given (model-view / projection) matrix.
  ///
  /// If the matrix is just the projection matrix, the frustum will be in local space. Pass the full ModelViewProjection
  /// matrix to create the frustum in world-space.
  ///
  /// The near plane will always be moved to the camera position, to prevent culling of objects between the camera position and
  /// the near plane (e.g. portals).
  /// The far plane will be taken from the matrix, but when it is farther away than fMaxFarPlaneDist (from the camera position),
  /// it will be moved closer to that distance.
  void SetFrustum(const ezVec3& vPosition, const ezMat4& ModelViewProjection, float fMaxFarPlaneDist);

  /// \brief Creates a frustum from the given camera position, direction vectors and the field-of-view along X and Y.
  ///
  /// The up vector does not need to be exactly orthogonal to the forwards vector, it will get recomputed properly.
  /// Fov X and Y define the entire field-of-view, so a Fov of 180 degree would mean the entire half-space in front of the camera.
  /// The near plane will always go through the camera position, to prevent culling objects between it and the camera.
  void SetFrustum(const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle FovX, ezAngle FovY, float fFarPlane);

  /// \brief Returns the number of planes used in this frustum.
  ezUInt8 GetNumPlanes() const; // [tested]

  /// \brief Returns the n-th plane of the frustum.
  const ezPlane& GetPlane(ezUInt8 uiPlane) const; // [tested]

  /// \brief Returns the start position of the frustum.
  const ezVec3& GetPosition() const; // [tested]

  /// \brief Transforms the frustum by the given matrix. This allows to adjust the frustum to a new orientation when a camera is moved or when it is necessary to cull from a different position.
  void TransformFrustum(const ezMat4& mTransform); // [tested]

  /// \brief Flips all frustum planes around. Might be necessary after creating the frustum from a mirror projection matrix.
  void InvertFrustum(); // [tested]

  /// \brief Checks whether the given object is inside or outside the frustum.
  ///
  /// A concave object might be classified as 'intersecting' although it is outside the frustum, if it overlaps the planes just right.
  /// However an object that is overlaps the frustum is definitely never classified as 'outside'.
  ezVolumePosition::Enum GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices) const;

  /// \brief Same as GetObjectPosition(), but applies a transformation to the given object first. This allows to do culling on instanced objects.
  ezVolumePosition::Enum GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices, const ezMat4& mObjectTransform) const;

  /// \brief Checks whether the given object is inside or outside the frustum.
  ezVolumePosition::Enum GetObjectPosition(const ezBoundingSphere& Sphere) const;

  /// \brief Checks whether the given object is inside or outside the frustum.
  ezVolumePosition::Enum GetObjectPosition(const ezBoundingBox& Box) const;


private:
  ezUInt8 m_uiUsedPlanes;
  ezVec3 m_vPosition;
  ezPlane m_Planes[16];
};

#include <Foundation/Math/Implementation/Frustum_inl.h>

