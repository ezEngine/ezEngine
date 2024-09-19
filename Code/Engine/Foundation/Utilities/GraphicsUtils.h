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
  EZ_FOUNDATION_DLL ezResult ConvertWorldPosToScreenPos(const ezMat4& mModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY,
    const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint, ezVec3& out_vScreenPos,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default); // [tested]

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
  EZ_FOUNDATION_DLL ezResult ConvertScreenPosToWorldPos(const ezMat4& mInverseModelViewProjection, const ezUInt32 uiViewportX,
    const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint,
    ezVec3* out_pDirection = nullptr,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief A double-precision version of ConvertScreenPosToWorldPos()
  EZ_FOUNDATION_DLL ezResult ConvertScreenPosToWorldPos(const ezMat4d& mInverseModelViewProjection, const ezUInt32 uiViewportX,
    const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint,
    ezVec3* out_pDirection = nullptr,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  EZ_FOUNDATION_DLL bool IsTriangleFlipRequired(const ezMat3& mTransformation);

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  EZ_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(
    ezMat4& inout_mMatrix, ezClipSpaceDepthRange::Enum srcDepthRange, ezClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  EZ_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, ezAngle& out_fovX, ezAngle& out_fovY); // [tested]

  /// \brief Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  EZ_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, ezAngle& out_fovLeft, ezAngle& out_fovRight, ezAngle& out_fovBottom, ezAngle& out_fovTop, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of ezGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa ezGraphicsUtils::CreatePerspectiveProjectionMatrix
  EZ_FOUNDATION_DLL ezResult ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns EZ_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  EZ_FOUNDATION_DLL ezResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const ezMat4& mProjectionMatrix,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default); // [tested]


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
  EZ_FOUNDATION_DLL ezPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const ezMat4& mProjectionMatrix,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrixFromFovX(ezAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  EZ_FOUNDATION_DLL ezMat4 CreatePerspectiveProjectionMatrixFromFovY(ezAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  EZ_FOUNDATION_DLL ezMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  EZ_FOUNDATION_DLL ezMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default, ezClipSpaceYMode::Enum range = ezClipSpaceYMode::Regular,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  EZ_FOUNDATION_DLL ezMat3 CreateLookAtViewMatrix(const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  EZ_FOUNDATION_DLL ezMat3 CreateInverseLookAtViewMatrix(const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix with both rotation and translation
  EZ_FOUNDATION_DLL ezMat4 CreateLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir,
    ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  EZ_FOUNDATION_DLL ezMat4 CreateInverseLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  EZ_FOUNDATION_DLL ezMat4 CreateViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  EZ_FOUNDATION_DLL ezMat4 CreateInverseViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  EZ_FOUNDATION_DLL void DecomposeViewMatrix(ezVec3& out_vPosition, ezVec3& out_vForwardDir, ezVec3& out_vRightDir, ezVec3& out_vUpDir, const ezMat4& mViewMatrix, ezHandedness::Enum handedness = ezHandedness::Default); // [tested]

  /// \brief Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns EZ_FAILURE.
  EZ_FOUNDATION_DLL ezResult ComputeBarycentricCoordinates(ezVec3& out_vCoordinates, const ezVec3& v0, const ezVec3& v1, const ezVec3& v2, const ezVec3& vPos);

  /// \brief Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns EZ_FAILURE.
  EZ_FOUNDATION_DLL ezResult ComputeBarycentricCoordinates(ezVec3& out_vCoordinates, const ezVec2& v0, const ezVec2& v1, const ezVec2& v2, const ezVec2& vPos);

  /// \brief Returns a coverage value of how much space a sphere at a given location would take up on screen using a perspective projection.
  ///
  /// The coverage value is close to 0 for very small or far away spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction, it only depends on distance and the camera's field-of-view.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera FOV angle is used for the calculation, pass in either the horizontal or vertical FOV angle,
  /// depending on what is most relevant to you.
  /// Typically the 'fixed' angle is used (usually the vertical one) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(const ezBoundingSphere& sphere, const ezVec3& vCameraPosition, ezAngle perspectiveCameraFov)
  {
    const float fDist = (sphere.m_vCenter - vCameraPosition).GetLength();
    const float fHalfHeight = ezMath::Tan(perspectiveCameraFov * 0.5f) * fDist;
    return sphere.m_fRadius / fHalfHeight;
  }

  /// \brief Returns a coverage value of how much space a sphere of a given size would take up on screen using an orthographic projection.
  ///
  /// The coverage value is close to 0 for very small spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction. In orthographic projections even the distance to the camera is irrelevant,
  /// only the dimensions of the ortho camera are needed.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera dimension is used for the calculation, pass in either the X or Y dimension, depending on what is most relevant to you.
  /// Typically the 'fixed' dimension is used (usually Y) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(float fSphereRadius, float fOrthoCameraDimensions)
  {
    const float fHalfHeight = fOrthoCameraDimensions * 0.5f;
    return fSphereRadius / fHalfHeight;
  }

} // namespace ezGraphicsUtils
