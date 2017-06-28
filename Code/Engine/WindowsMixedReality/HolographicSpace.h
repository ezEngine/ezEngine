#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>


namespace ABI
{
  namespace Windows
  {
    namespace Foundation
    {
      struct IDeferral;
    }
    namespace Graphics
    {
      namespace Holographic
      {
        struct IHolographicSpaceStatics;
        struct IHolographicSpace;

        struct IHolographicSpaceCameraAddedEventArgs;
        struct IHolographicSpaceCameraRemovedEventArgs;

        struct IHolographicCamera;
        struct IHolographicFrame;
      }
    }
  }
}

class ezWindowsSpatialLocationService;
class ezWindowsMixedRealityCamera;
struct IDXGIAdapter3;

/// \brief Integration of Windows HolographicSpace (WinRT).
///
/// Right now window holographic is the only vr/mr platform we support.
/// In the future this may be the implementation of an abstract interface that allows the use of arbitrary vr/mr device.
/// For now however, this is entirely platform specific.
///
/// The holographic space is an important entry point for:
/// * Communication with the device (see MixedRealityDX11Device)
/// * Adding/removing/updating holographic cameras (see MixedRealityCamera)
/// * General information about mixed reality hardware
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

  /// Wheather vr/mr headsets are supported at all.
  ///
  /// True for all x64 Windows beyond Creator's Update if the headset was setup already. 
  //bool IsSupported() const;

  /// \brief Wheather a headset is ready for rendering.
  ///
  /// Can be called *before* being initialized with a window.
  bool IsAvailable() const;


  // Cameras
public:

  /// \brief Processes queued holographic camera additions and removals.
  ///
  /// Needs to be called by the user in order to create new cameras and corresponding swap chains.
  /// If a new camera was added or removed the corresponding events are fired.
  void ProcessAddedRemovedCameras();

  /// \brief Gets list of all cameras.
  ezArrayPtr<ezWindowsMixedRealityCamera*> GetCameras() { return ezMakeArrayPtr(m_cameras); }

  // ezEvents for camera add/remove. Always fired on rendering thread.
  ezEvent<const ezWindowsMixedRealityCamera&> m_cameraAddedEvent;
  ezEvent<const ezWindowsMixedRealityCamera&> m_cameraRemovedEvent;

  // Internal
public:

  /// \brief Called by holographic device.
  ///
  /// Applies deferred camera add/remove and updates camera poses.
  /// May change swap chain backbuffer sizes.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame> StartNewHolographicFrame();

  ABI::Windows::Graphics::Holographic::IHolographicSpace* GetInternalHolographicSpace() { return m_pHolographicSpace.Get(); }


private:

  void DeInit();

  /// \brief Updates all camera poses from holographic frame.
  ezResult UpdateCameraPoses(const ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame>& pHolographicFrame);

  // Callbacks for camera add/remove
  HRESULT OnCameraAdded(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraAddedEventArgs* args);
  HRESULT OnCameraRemoved(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraRemovedEventArgs* args);




  /// Static holographic space access. Created on startup. If this is null nothing else will work.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics> m_pHolographicSpaceStatics;
  /// Windows holographic space, created in init method for a specific window.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpace> m_pHolographicSpace;

  ezUniquePtr<ezWindowsSpatialLocationService> m_pDefaultLocationService;

  // Camera subscriptions on holographic space.
  EventRegistrationToken m_eventRegistrationOnCameraAdded;
  EventRegistrationToken m_eventRegistrationOnCameraRemoved;

  struct PendingCameraAddition
  {
    ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> m_pCamera;
    ComPtr<ABI::Windows::Foundation::IDeferral> m_pDeferral;
  };

  ezMutex m_cameraQueueMutex;
  ezDynamicArray<ezWindowsMixedRealityCamera*> m_cameras;
  ezDynamicArray<ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera>> m_pendingCameraRemovals;
  ezDynamicArray<PendingCameraAddition> m_pendingCameraAdditions;
};

