#pragma once

#include <Foundation/Math/Rect.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/Declarations.h>

/// \brief Holds view data like the viewport, view and projection matrices
struct EZ_RENDERERCORE_DLL ezViewData
{
  ezViewData()
  {
    m_ViewPortRect = ezRectFloat(0.0f, 0.0f);

    m_ViewMatrix.SetIdentity();
    m_InverseViewMatrix.SetIdentity();
    m_ProjectionMatrix.SetIdentity();
    m_InverseProjectionMatrix.SetIdentity();
    m_ViewProjectionMatrix.SetIdentity();
    m_InverseViewProjectionMatrix.SetIdentity();
  }

  ezRectFloat m_ViewPortRect;

  ezMat4 m_ViewMatrix;
  ezMat4 m_InverseViewMatrix;
  ezMat4 m_ProjectionMatrix;
  ezMat4 m_InverseProjectionMatrix;
  ezMat4 m_ViewProjectionMatrix;
  ezMat4 m_InverseViewProjectionMatrix;

  /// \brief Returns the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fScreenPosX and fScreenPosY are expected to be in [0; 1] range (normalized pixel coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  ezResult ComputePickingRay(float fScreenPosX, float fScreenPosY, ezVec3& out_RayStartPos, ezVec3& out_RayDir)
  {
    ezVec3 vScreenPos;
    vScreenPos.x = fScreenPosX;
    vScreenPos.y = 1.0f - fScreenPosY;
    vScreenPos.z = 0.0f;

    return ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix, 0, 0, 1, 1, vScreenPos, out_RayStartPos, &out_RayDir);
  }
};
