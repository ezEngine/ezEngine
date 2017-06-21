#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>


namespace ABI
{
  namespace Windows
  {
    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicSpaceStatics;
        struct IHolographicSpace;

        struct IHolographicSpaceCameraAddedEventArgs;
        struct IHolographicSpaceCameraRemovedEventArgs;
      }

      namespace DirectX
      {
        namespace Direct3D11
        {
          struct IDirect3DDevice;
        }
      }
    }
  }
}

class ezWindowsHolographicLocationService;
struct IDXGIAdapter3;

/// \brief Integration of Windows HolographicSpace (WinRT).
///
/// Right now window holographic is the only vr/mr platform we support.
/// In the future this may be the implementation of an abstract interface that allows the use of arbitrary vr/mr device.
/// For now however, this is entirely platform specific.
class EZ_WINDOWSMIXEDREALITY_DLL ezWindowsHolographicSpace
{
  EZ_DECLARE_SINGLETON(ezWindowsHolographicSpace);

public:
  ezWindowsHolographicSpace();
  ~ezWindowsHolographicSpace();

  /// \brief Initializes the holographic space for the main core window. Called automatically on startup.
  ///
  /// The holographic space requires a window but also determines the DXGI adpater from which we need to create our DX11 device.
  /// Historically we first create a device and *then* the window.
  /// However, since in a VR/AR application we have only a single window anyway and UWP does always have a main window, we can just query that one and don't need to fiddle with our init order.
  /// (as of writing our UWP window implementation actually doesn't support more than this one preexisting window
  ezResult InitForMainCoreWindow();

  /// Returns the DXGI adapter that should be used for device creation.
  IDXGIAdapter3* GetDxgiAdapter() const { return m_pDxgiAdapter.Get(); }

  /// \brief Set GAL default device for the holographic space.
  ezResult SetDX11Device();

  /// Wheather vr/mr headsets are supported at all.
  ///
  /// True for all x64 Windows beyond Creator's Update if the headset was setup already. 
  //bool IsSupported() const;

  /// \brief Wheather a headset is ready for rendering.
  ///
  /// Can be called *before* being initialized with a window.
  bool IsAvailable() const;

private:

  void DeInit();

  HRESULT OnCameraAdded(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraAddedEventArgs* args);
  HRESULT OnCameraRemoved(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraRemovedEventArgs* args);


  /// Static holographic space access. Created on startup. If this is null nothing else will work.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics> m_pHolographicSpaceStatics;
  /// Windows holographic space, created in init method for a specific window.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpace> m_pHolographicSpace;

  // Camera subscriptions on holographic space.
  EventRegistrationToken m_eventRegistrationOnCameraAdded;
  EventRegistrationToken m_eventRegistrationOnCameraRemoved;

  /// DXGI adapter given by holographic space.
  Microsoft::WRL::ComPtr<IDXGIAdapter3> m_pDxgiAdapter;
  /// DX11 interop device with current ezGALDX11 device.
  ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> m_pDX11InteropDevice;


  ezUniquePtr<ezWindowsHolographicLocationService> m_pDefaultLocationService;
};

