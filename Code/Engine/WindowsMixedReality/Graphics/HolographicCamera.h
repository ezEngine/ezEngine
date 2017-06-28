
#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/Resources/ResourceFormats.h>

namespace ABI
{
  namespace Windows
  {
    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicCamera;
        struct IHolographicCameraPose;
        struct IHolographicCameraRenderingParameters;
      }
    }
  }
}

/// \brief Represents a camera in windows holographic.
///
/// Each camera is associated with a ezGALHolographicSwapChainDX11.
/// \see ezGALHolographicSwapChainDX11
class ezWindowsHolographicCamera
{
public:
  ezWindowsHolographicCamera(const ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> pHolographicCamera);
  ~ezWindowsHolographicCamera();

  /// \brief Get unique identifier of this camera.
  ezUInt32 GetId() const;

  // Todo: Expose
  // * set far/near plane
  // * left/right viewport parameters (hidden/visible area mesh - creator's update feature for VR, IHolographicCamera2)
  // * meta data like display name (creator's update, IHolographicCamera2)

  // Backbuffer properties. These can change every frame!
  ezRectU32 GetBackBufferSize() const;
  bool IsStereoscopic() const;

  // Pose information
public:

  /// \brief Gets project matrix for the left eye for the active frame.
  ///
  /// If this is not a stereoscopic camera, left and right projection matrices are identical.
  const ezMat4& GetProjectionLeft() const { return m_projectionMatrices[0]; }

  /// \brief Gets project matrix for the left eye for the active frame.
  ///
  /// If this is not a stereoscopic camera, left and right projection matrices are identical.
  const ezMat4& GetProjectionRight() const { return m_projectionMatrices[1]; }

  /// \brief Returns the viewport rectangle the app must render to for the active frame.
  const ezRectFloat GetViewport() const { return m_viewport; }

  // Internal
public:

  /// \brief Updates camera pose and swapchain if necessary.
  HRESULT UpdatePose(ABI::Windows::Graphics::Holographic::IHolographicCameraPose* pPose,
                     ABI::Windows::Graphics::Holographic::IHolographicCameraRenderingParameters* pRenderingParameters);

  ABI::Windows::Graphics::Holographic::IHolographicCamera* GetInternalHolographicCamera() const { return m_pHolographicCamera.Get(); }

private:

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> m_pHolographicCamera;
  ezGALSwapChainHandle m_associatedSwapChain;

  ezMat4 m_projectionMatrices[2];
  ezRectFloat m_viewport;
};
