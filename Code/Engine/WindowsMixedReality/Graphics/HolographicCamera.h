
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

  /// Get unique identifier of this camera.
  ezUInt32 GetId() const;

  // Todo: Expose
  // * set far/near plane
  // * left/right viewport parameters (hidden/visible area mesh - creator's update feature for VR, IHolographicCamera2)
  // * meta data like display name (creator's update, IHolographicCamera2)

  // Backbuffer properties. These can change every frame!
  ezRectU32 GetBackBufferSize() const;
  bool IsStereoscopic() const;


  ABI::Windows::Graphics::Holographic::IHolographicCamera* GetInternalHolographicCamera() const { return m_pHolographicCamera.Get(); }

private:

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> m_pHolographicCamera;
  ezGALSwapChainHandle m_associatedSwapChain;
};
