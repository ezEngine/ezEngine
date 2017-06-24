#pragma once

#include <WindowsMixedReality/Basics.h>
#include <RendererDX11/Device/DeviceDX11.h>


namespace ABI
{
  namespace Windows
  {
    namespace Graphics
    {
      namespace DirectX
      {
        namespace Direct3D11
        {
          struct IDirect3DDevice;
        }
      }
      namespace Holographic
      {
        struct IHolographicFrame;
      }
    }
  }
}

/// \brief A specialization of ezGALDeviceDX11 for windows holographic.
///
/// On hololens, swapchain generation and frame begin/end/present is very different from a normal DX11 device.
/// To avoid many special hooks and hacks in the standard DX11 device, we inherit from ezGALDeviceDX11 instead.
/// This allows the necessary coupling with HolographicSpace without needing the user to handle this communication.
///
/// Other than that this is practically identical to a normal DX11 device.
///
/// Note that there can only be ever a single holographic DX11 device in existance!
class EZ_WINDOWSMIXEDREALITY_DLL ezGALHolographicDeviceDX11 : public ezGALDeviceDX11
{
  /// \todo This shouldn't be accessible, there should be a factory instantiating the correct renderer class via RTTI for example
public:

  ezGALHolographicDeviceDX11(const ezGALDeviceCreationDescription& Description);

  virtual ~ezGALHolographicDeviceDX11();

protected:

  virtual ezResult InitPlatform() override;

  virtual ezResult ShutdownPlatform() override;

  virtual ezGALSwapChain* CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description) override;

  virtual void PresentPlatform(ezGALSwapChain* pSwapChain) override;

  // Misc functions

  virtual void BeginFramePlatform() override;

  virtual void EndFramePlatform() override;

private:

  /// WinRT DX11 interop device with current ezGALDX11 device.
  ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> m_pDX11InteropDevice;

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame> m_pCurrentHolographicFrame;
};

