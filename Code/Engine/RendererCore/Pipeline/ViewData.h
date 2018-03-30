#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <Core/Graphics/Camera.h>

/// \brief Holds view data like the viewport, view and projection matrices
struct EZ_RENDERERCORE_DLL ezViewData
{
  ezViewData()
  {
    m_ViewPortRect = ezRectFloat(0.0f, 0.0f);
    m_ViewRenderMode = ezViewRenderMode::None;

    for (int i = 0; i < 2; ++i)
    {
      m_ViewMatrix[i].SetIdentity();
      m_InverseViewMatrix[i].SetIdentity();
      m_ProjectionMatrix[i].SetIdentity();
      m_InverseProjectionMatrix[i].SetIdentity();
      m_ViewProjectionMatrix[i].SetIdentity();
      m_InverseViewProjectionMatrix[i].SetIdentity();
    }
  }

  ezRectFloat m_ViewPortRect;
  ezEnum<ezViewRenderMode> m_ViewRenderMode;

  // Each matrix is there for both left and right camera lens.
  ezMat4 m_ViewMatrix[2];
  ezMat4 m_InverseViewMatrix[2];
  ezMat4 m_ProjectionMatrix[2];
  ezMat4 m_InverseProjectionMatrix[2];
  ezMat4 m_ViewProjectionMatrix[2];
  ezMat4 m_InverseViewProjectionMatrix[2];

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  ezResult ComputePickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir, ezCameraEye eye = ezCameraEye::Left) const
  {
    ezVec3 vScreenPos;
    vScreenPos.x = fScreenPosX;
    vScreenPos.y = 1.0f - fScreenPosY;
    vScreenPos.z = 0.0f;

    return ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<int>(eye)], 0, 0, 1, 1, vScreenPos, out_RayStartPos, &out_RayDir);
  }

  ezResult ComputeScreenSpacePos(const ezVec3& vPoint, ezVec3& out_vScreenPos, ezCameraEye eye = ezCameraEye::Left) const
  {
    ezUInt32 x = (ezUInt32)m_ViewPortRect.x;
    ezUInt32 y = (ezUInt32)m_ViewPortRect.y;
    ezUInt32 w = (ezUInt32)m_ViewPortRect.width;
    ezUInt32 h = (ezUInt32)m_ViewPortRect.height;

    if (ezGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], x, y, w, h, vPoint, out_vScreenPos).Succeeded())
    {
      out_vScreenPos.y = m_ViewPortRect.height - out_vScreenPos.y;

      return EZ_SUCCESS;
    }

    return EZ_FAILURE;
  }
};

