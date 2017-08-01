#pragma once

#include <WindowsMixedReality/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>

class ezCamera;
struct ezGameApplicationEvent;

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

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, WindowsHolographicSpace);

public:
  ezWindowsHolographicSpace();
  ~ezWindowsHolographicSpace();

  /// \brief Makes sure a static reference frame exists, and optionally sets an ezCamera instance for automatic synchronization.
  void Activate(ezCamera* pCameraForSynchronization /* = nullptr*/);

  /// whether VR/MR headsets are supported at all.
  ///
  /// True for all x64 Windows beyond Creator's Update if the headset was setup already. 
  //bool IsSupported() const;

  /// \brief whether a headset is ready for rendering.
  ///
  /// Can be called *before* being initialized with a window.
  bool IsAvailable() const;

  /// \brief Returns spatial location service that allows to create reference frames.
  ///
  /// The location service is fully owned and managed by this class.
  ezWindowsSpatialLocationService& GetSpatialLocationService() { return *m_pLocationService; }

  /// \brief Sets the spatial reference frame that is used as the default reference frame.
  void SetDefaultReferenceFrame(ezUniquePtr<ezWindowsSpatialReferenceFrame>&& refFrame);

  /// \brief Returns the spatial reference frame that is used as the default reference frame.
  const ezWindowsSpatialReferenceFrame* GetDefaultReferenceFrame() const;

  /// \brief Returns the last prediction timestamp. Some APIs need this timestamp to query data at a consistent time.
  const ComPtr<ABI::Windows::Perception::IPerceptionTimestamp>& GetPredictionTimestamp() const { return m_pPredictionTimestamp; }

  // Cameras
public:

  /// \brief Gets list of all cameras.
  ezArrayPtr<ezWindowsMixedRealityCamera*> GetCameras() { return ezMakeArrayPtr(m_cameras); }

  /// \brief When an ezCamera is provided, the holographic space will automatically fill it with the
  /// latest prediction values at the right time every frame.
  void SetCameraForPredictionSynchronization(ezCamera* pCamera);

  /// \brief Retrieves the current head tracking camera data and fills out the corresponding ezCamera values.
  ezResult SynchronizeCameraPrediction(ezCamera& inout_Camera);

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

  //
  // Initialization
  //

  /// \brief Initializes the holographic space for the main core window. Called automatically on startup.
  ///
  /// The holographic space requires a window but also determines the DXGI adapter from which we need to create our DX11 device.
  /// Historically we first create a device and *then* the window.
  /// However, since in a VR/AR application we have only a single window anyway and UWP does always have a main window, we can just query that one and don't need to fiddle with our init order.
  /// (as of writing our UWP window implementation actually doesn't support more than this one preexisting window
  ezResult InitForMainCoreWindow();

  void DeInit();

  /// Disables multi-threaded rendering to reduce frame lag, which has a significant impact on Hologram stability
  static void DisableMultiThreadedRendering();

  // implements automatic camera synchronization among other things
  void GameApplicationEventHandler(const ezGameApplicationEvent& e);

  bool m_bAddedGameAppEventHandler = false;

  /// Static holographic space access. Created on startup. If this is null nothing else will work.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics> m_pHolographicSpaceStatics;
  /// Windows holographic space, created in init method for a specific window.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpace> m_pHolographicSpace;

  /// The timestamp at which the last frame was started and therefore all predictions where done.
  ComPtr<ABI::Windows::Perception::IPerceptionTimestamp> m_pPredictionTimestamp;

  //
  // Camera
  //

  /// \brief Processes queued holographic camera additions and removals.
  void ProcessAddedRemovedCameras();

  /// \brief Updates all camera poses from holographic frame.
  ezResult UpdateCameraPoses(const ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame>& pHolographicFrame);

  // Callbacks for camera add/remove
  HRESULT OnCameraAdded(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraAddedEventArgs* args);
  HRESULT OnCameraRemoved(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraRemovedEventArgs* args);
  

  // the camera object that will get the head tracking data applied every frame
  ezCamera* m_pCameraToSynchronize = nullptr;

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

  //
  // Location
  //

  /// \brief Creates a default reference frame at the current location of the device, if none exists yet.
  void CreateDefaultReferenceFrame();

  ezUniquePtr<ezWindowsSpatialLocationService> m_pLocationService;
  ezUniquePtr<ezWindowsSpatialReferenceFrame> m_pDefaultReferenceFrame;
};

