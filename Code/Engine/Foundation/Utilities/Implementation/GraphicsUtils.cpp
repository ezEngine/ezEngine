#include <Foundation/PCH.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Math/Vec4.h>

ezResult ezGraphicsUtils::ConvertWorldPosToScreenPos(const ezMat4& ModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint, ezVec3& out_vScreenPos, ezProjectionDepthRange::Enum DepthRange)
{
  const ezVec4 vToProject = vPoint.GetAsVec4(1.0f);

  ezVec4 vClipSpace = ModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f) 
    return EZ_FAILURE;

  const ezVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;

  out_vScreenPos.x = uiViewportX + uiViewportWidth  * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ConvertScreenPosToWorldPos(const ezMat4& InverseModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint, ezVec3* out_vDirection, ezProjectionDepthRange::Enum DepthRange)
{
  ezVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  ezVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  ezVec4 vWorldSpacePoint = InverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return EZ_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_vDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const ezVec4 vWorldSpacePoint2 = InverseModelViewProjection * vToUnProject;

    EZ_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const ezVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_vDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return EZ_SUCCESS;
}





EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_GraphicsUtils);

