#pragma once

#include <Foundation/Math/Mat4.h>

namespace ezGraphicsUtils
{
  /// \brief Projects the given point from 3D world space into screen space, if possible.
  ///
  /// \param ModelViewProjection
  ///   The Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see ezProjectionDepthRange
  ///
  /// Returns EZ_FAILURE, if the point could not be projected into screen space.
  /// \note The function reports EZ_SUCCESS, when the point could be projected, however, that does not mean that the point actually lies
  /// within the viewport, it might still be outside the viewport.
  ///
  /// out_vScreenPos.z is the depth of the point in [0;1] range. The z value is always 'normalized' to this range 
  /// (as long as the DepthRange parameter is correct), to make it easier to make subsequent code platform independent.
  EZ_FOUNDATION_DLL ezResult ConvertWorldPosToScreenPos(const ezMat4& ModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint, ezVec3& out_vScreenPos, ezProjectionDepthRange::Enum DepthRange = ezProjectionDepthRange::Default); // [tested]

  /// \brief Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
  ///
  /// \param InverseModelViewProjection
  ///   The inverse of the Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see ezProjectionDepthRange
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
  EZ_FOUNDATION_DLL ezResult ConvertScreenPosToWorldPos(const ezMat4& InverseModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint, ezVec3* out_vDirection = nullptr, ezProjectionDepthRange::Enum DepthRange = ezProjectionDepthRange::Default); // [tested]
}


