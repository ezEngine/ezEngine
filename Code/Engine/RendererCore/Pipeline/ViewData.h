#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Math/Size.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>
#include <RendererFoundation/Device/SwapChain.h>

/// Holds view data like the viewport, view and projection matrices
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

  ezHashedString m_sName;
  ezUInt32 m_uiSkyIrradianceIndex = 0;

  ezGALRenderTargets m_RenderTargets;
  ezGALSwapChainHandle m_hSwapChain;
  ezRectFloat m_ViewPortRect;

  /// Factor applied to the viewport size for render targets of source passes that have 'ApplyRenderScale' enabled.
  ///
  /// Used to render the scene at a lower resolution than the output. The pipeline needs an ezUpscalePass to bring the
  /// result back to the viewport size, otherwise the scaled textures reach the output with the wrong size.
  float m_fRenderScale = 1.0f;

  ezEnum<ezViewRenderMode> m_ViewRenderMode;
  ezEnum<ezCameraUsageHint> m_CameraUsageHint;

  // Each matrix is there for both left and right camera lens.
  ezMat4 m_ViewMatrix[2];
  ezMat4 m_InverseViewMatrix[2];
  ezMat4 m_ProjectionMatrix[2];
  ezMat4 m_InverseProjectionMatrix[2];
  ezMat4 m_ViewProjectionMatrix[2];
  ezMat4 m_InverseViewProjectionMatrix[2];

  /// Calculates the start position and direction (in world space) of the picking ray through the screen position in this view.
  ///
  /// fNormalizedScreenPosX and fNormalizedScreenPosY are expected to be in [0; 1] range (normalized screen coordinates).
  /// If no ray can be computed, EZ_FAILURE is returned.
  EZ_ALWAYS_INLINE ezResult ComputePickingRay(float fNormalizedScreenPosX, float fNormalizedScreenPosY, ezVec3& out_vRayStartPos, ezVec3& out_vRayDir, ezCameraEye eye = ezCameraEye::Left) const
  {
    ezVec3 vScreenPos;
    vScreenPos.x = fNormalizedScreenPosX;
    vScreenPos.y = fNormalizedScreenPosY;
    vScreenPos.z = 0.0f;

    return ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<int>(eye)], vScreenPos, out_vRayStartPos, &out_vRayDir);
  }

  /// Calculates the normalized screen-space coordinate ([0; 1] range) that the given world-space point projects to.
  ///
  /// Returns EZ_FAILURE, if the point could not be projected into screen-space.
  EZ_ALWAYS_INLINE ezResult ComputeScreenSpacePos(const ezVec3& vWorldPos, ezVec3& out_vScreenPosNormalized, ezCameraEye eye = ezCameraEye::Left) const
  {
    return ezGraphicsUtils::ConvertWorldPosToScreenPos(m_ViewProjectionMatrix[static_cast<int>(eye)], vWorldPos, out_vScreenPosNormalized);
  }

  /// Calculates the world-space position that the given normalized screen-space coordinate maps to
  EZ_ALWAYS_INLINE ezResult ComputeWorldSpacePos(float fNormalizedScreenPosX, float fNormalizedScreenPosY, ezVec3& out_vWorldPos, ezCameraEye eye = ezCameraEye::Left) const
  {
    return ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseViewProjectionMatrix[static_cast<int>(eye)], ezVec3(fNormalizedScreenPosX, fNormalizedScreenPosY, 0.0f), out_vWorldPos);
  }

  /// Converts a screen-space position from pixel coordinates to normalized coordinates.
  EZ_ALWAYS_INLINE void ConvertScreenPixelPosToNormalizedPos(ezVec3& inout_vPixelPos) const
  {
    ezUInt32 x = (ezUInt32)m_ViewPortRect.x;
    ezUInt32 y = (ezUInt32)m_ViewPortRect.y;
    ezUInt32 w = (ezUInt32)m_ViewPortRect.width;
    ezUInt32 h = (ezUInt32)m_ViewPortRect.height;
    ezGraphicsUtils::ConvertScreenPixelPosToNormalizedPos(x, y, w, h, inout_vPixelPos);
  }

  /// Returns the viewport size scaled by m_fRenderScale. Each dimension is at least one pixel.
  ezSizeU32 GetScaledViewportSize() const
  {
    const float fScale = ezMath::Clamp(m_fRenderScale, 0.01f, 1.0f);
    const ezUInt32 uiWidth = ezMath::Max(1u, static_cast<ezUInt32>(ezMath::Round(m_ViewPortRect.width * fScale)));
    const ezUInt32 uiHeight = ezMath::Max(1u, static_cast<ezUInt32>(ezMath::Round(m_ViewPortRect.height * fScale)));
    return ezSizeU32(uiWidth, uiHeight);
  }

  /// Returns the viewport to use when rendering into targets of the given size.
  ///
  /// This is m_ViewPortRect if it fits into the targets, which is the case for the view's own render targets and for
  /// intermediate textures of the same size. Smaller targets, such as the ones created with a render scale, get a viewport
  /// that covers the entire target instead, because the view's viewport would be clipped.
  ezRectFloat GetViewportForTargetSize(ezSizeU32 targetSize) const
  {
    if (!targetSize.HasNonZeroArea() || (m_ViewPortRect.Right() <= static_cast<float>(targetSize.width) && m_ViewPortRect.Bottom() <= static_cast<float>(targetSize.height)))
      return m_ViewPortRect;

    return ezRectFloat(0.0f, 0.0f, static_cast<float>(targetSize.width), static_cast<float>(targetSize.height));
  }

  /// Returns the active render targets. If a swap chain is set, its render targets are returned, otherwise m_RenderTargets.
  const ezGALRenderTargets& GetActiveRenderTargets() const;

  /// Converts a screen-space position from normalized coordinates to pixel coordinates.
  EZ_ALWAYS_INLINE void ConvertScreenNormalizedPosToPixelPos(ezVec3& inout_vNormalizedPos) const
  {
    {
      ezUInt32 x = (ezUInt32)m_ViewPortRect.x;
      ezUInt32 y = (ezUInt32)m_ViewPortRect.y;
      ezUInt32 w = (ezUInt32)m_ViewPortRect.width;
      ezUInt32 h = (ezUInt32)m_ViewPortRect.height;
      ezGraphicsUtils::ConvertScreenNormalizedPosToPixelPos(x, y, w, h, inout_vNormalizedPos);
    }
  }
};
