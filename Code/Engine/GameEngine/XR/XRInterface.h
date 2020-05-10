#pragma once

#include <GameEngine/ActorSystem/Actor.h>
#include <GameEngine/XR/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Math/Size.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;
class ezViewHandle;
class ezCamera;
class ezGALTextureHandle;
class ezWorld;
class ezXRInterface;
class ezView;
class ezXRInputDevice;

struct ezHMDInfo
{
  ezString m_sDeviceName;
  ezString m_sDeviceDriver;
  ezSizeU32 m_vEyeRenderTargetSize;
};

/// \brief Defines the stage space used for the XR experience.
///
/// This value is set by the ezStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct ezXRStageSpace
{
  typedef ezUInt8 StorageType;
  enum Enum : ezUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRStageSpace);

class ezWindowBase;
class ezWindowOutputTargetBase;

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

  /// \brief Called by ezWindowOutputTargetXR::Present
  /// Returns the color texture to be used by the companion view if enabled, otherwise an invalid handle.
  virtual ezGALTextureHandle Present() = 0;

  virtual ezUniquePtr<ezActor> CreateActor(
    ezView* pView, ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None,
    ezUniquePtr<ezWindowBase> companionWindow = nullptr, ezUniquePtr<ezWindowOutputTargetBase> companionWindowOutput = nullptr) = 0;
  /// \brief Called when the actor created by 'CreateActor' is destroyed.
  virtual void OnActorDestroyed() = 0;

  /// \brief Returns true if SetCompanionViewRenderTarget is supported.
  virtual bool SupportsCompanionView() = 0;

  ///@}
};
