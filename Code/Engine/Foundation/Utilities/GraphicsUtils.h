#pragma once

#include <Foundation/Math/Mat4.h>

namespace ezGraphicsUtils
{
  /// \brief Projects the given point from 3D world space into screen space, if possible.
  ///
  /// \param ModelViewProjection
  ///   The Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see ezClipSpaceDepthRange
  ///
  /// Returns EZ_FAILURE, if the point could not be projected into screen space.
  /// \note The function reports EZ_SUCCESS, when the point could be projected, however, that does not mean that the point actually lies
  /// within the viewport, it might still be outside the viewport.
  ///
  /// out_vScreenPos.z is the depth of the point in [0;1] range. The z value is always 'normalized' to this range
  /// (as long as the DepthRange parameter is correct), to make it easier to make subsequent code platform independent.
  EZ_FOUNDATION_DLL ezResult ConvertWorldPosToScreenPos(const ezMat4& ModelViewProjection, const ezUInt32 uiViewportX,
    const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint,
    ezVec3& out_vScreenPos,
    ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
  ///
  /// \param InverseModelViewProjection
  ///   The inverse of the Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see ezClipSpaceDepthRange
  ///
  /// Returns EZ_FAILURE when the screen coordinate could not be converted to a world position,
  /// which should generally not be possible as long as the coordinate is actually inside the viewport.
  ///
  /// Optionally this function also computes the direction vector through the world space position, that should be used for picking
  /// operations. Note that for perspective cameras this is the same as the direction from the camera position to the computed point,
  /// but for orthographic cameras it is not (it's simply the forward vector of the camera).
  /// This function handles both cases properly.
  ///
  /// The z value of vScreenPos is always expected to be in [0; 1] range (meaning 0 is at the near plane, 1 at the far plane),
  /// even on platforms that use [-1; +1] range for clip-space z values. The DepthRange parameter needs to be correct to handle this case
  /// properly.
  EZ_FOUNDATION_DLL ezResult ConvertScreenPosToWorldPos(const ezMat4& InverseModelViewProjection, const ezUInt32 uiViewportX,
    const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos,
    ezVec3& out_vPoint, ezVec3* out_vDirection = nullptr,
    ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief A double-precision version of ConvertScreenPosToWorldPos()
  EZ_FOUNDATION_DLL ezResult ConvertScreenPosToWorldPos(const ezMat4d& InverseModelViewProjection, const ezUInt32 uiViewportX,
    const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos,
    ezVec3& out_vPoint, ezVec3* out_vDirection = nullptr,
    ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  EZ_FOUNDATION_DLL bool IsTriangleFlipRequired(const ezMat3& mTransformation);

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  EZ_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(
    ezMat4& inout_Matrix, ezClipSpaceDepthRange::Enum SrcDepthRange, ezClipSpaceDepthRange::Enum DstDepthRange); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  EZ_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(
    const ezMat4& ProjectionMatrix, ezAngle& out_fFovX, ezAngle& out_fFovY); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  EZ_FOUNDATION_DLL void ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const ezMat4& ProjectionMatrix,
    ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default); // [tested]


  enum class FrustumPlaneInterpolation
  {
    LeftToRight,
    BottomToTop,
    NearToFar,
  };

  /// \brief Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  EZ_FOUNDATION_DLL ezPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor,
    const ezMat4& ProjectionMatrix, ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrixFromFovX(ezAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrixFromFovY(ezAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum DepthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  EZ_FOUNDATION_DLL ezMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  EZ_FOUNDATION_DLL ezMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default,
    ezClipSpaceYMode::Enum yRange = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  EZ_FOUNDATION_DLL ezMat3 CreateLookAtViewMatrix(
    const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  EZ_FOUNDATION_DLL ezMat3 CreateInverseLookAtViewMatrix(
    const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]


  /// \brief Returns a look-at matrix with both rotation and translation
  EZ_FOUNDATION_DLL ezMat4 CreateLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  EZ_FOUNDATION_DLL ezMat4 CreateInverseLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  EZ_FOUNDATION_DLL ezMat4 CreateViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir,
    const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  EZ_FOUNDATION_DLL ezMat4 CreateInverseViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir,
    const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  EZ_FOUNDATION_DLL void DecomposeViewMatrix(ezVec3& out_vPosition, ezVec3& out_vForwardDir, ezVec3& out_vRightDir, ezVec3& out_vUpDir,
    const ezMat4& viewMatrix, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]


} // namespace ezGraphicsUtils
