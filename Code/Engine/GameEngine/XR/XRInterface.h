#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/System/WindowManager.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using ezRenderPipelineResourceHandle = ezTypedResourceHandle<class ezRenderPipelineResource>;

class ezViewHandle;
class ezCamera;
class ezGALTextureHandle;
class ezWorld;
class ezView;
class ezXRInputDevice;
class ezWindowBase;
class ezWindowOutputTargetBase;
struct ezGALMSAASampleCount;

/// \brief XR singleton interface. Needs to be initialized to be used for VR or AR purposes.
///
/// To be used in a project the project needs to have an enabled ezXRConfig with a
/// set render pipeline in the platform profile.
/// To then use the interface, Initialize must be called first and on success CreateActor.
/// Everything else is optional.
/// Aquire interface via ezSingletonRegistry::GetSingletonInstance<ezXRInterface>().
class ezXRInterface
{
public:
  /// \brief Returns whether an HMD is available. Can be used to decide
  /// whether it makes sense to call Initialize at all.
  virtual bool IsHmdPresent() const = 0;

  /// \name Setup
  ///@{

  /// \brief Initializes the XR system. This can be quite time consuming
  /// as it will generally start supporting applications needed to run
  /// and start up the HMD if it went to sleep.
  virtual ezResult Initialize() = 0;
  /// \brief Shuts down the XR system again.
  virtual void Deinitialize() = 0;
  /// \brief Returns whether the XR system is initialized.
  virtual bool IsInitialized() const = 0;

  ///@}
  /// \name Devices
  ///@{

  /// \brief Returns general HMD information.
  virtual const ezHMDInfo& GetHmdInfo() const = 0;

  /// \brief Returns the XR input device.
  virtual ezXRInputDevice& GetXRInput() const = 0;

  ///@}
  /// \name View
  ///@{

  /// \brief Returns true if a companion window can be passed into CreateActor.
  virtual bool SupportsCompanionView() = 0;

  /// \brief Creates a XR actor by trying to startup an XR session.
  ///
  /// If SupportsCompanionView is true (VR only), a normal window and window output can be passed in.
  /// The window will be used to blit the VR output into the window.
  virtual ezRegisteredWndHandle CreateXRWindow(ezView* pView, ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None, ezUniquePtr<ezWindowBase> pCompanionWindow = nullptr, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) = 0;

  ///@}
  /// \name Internal
  ///@{

  /// \brief Called by ezWindowOutputTargetXR::RenderCompanionView
  /// Returns the color texture to be used by the companion view if enabled, otherwise an invalid handle.
  virtual ezGALTextureHandle GetCurrentTexture() = 0;

  /// \brief Called when the actor created by 'CreateActor' is destroyed.
  virtual void OnActorDestroyed() = 0;

  ///@}
};
