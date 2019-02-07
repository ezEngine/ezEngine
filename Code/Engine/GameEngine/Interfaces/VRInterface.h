#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/GameEngineDLL.h>
#include <RendererFoundation/RendererFoundationDLL.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;
class ezViewHandle;
class ezCamera;
class ezGALTextureHandle;
class ezWorld;

struct ezHMDInfo
{
  ezString m_sDeviceName;
  ezString m_sDeviceDriver;
  ezMat4 m_mat4eyePosLeft;
  ezMat4 m_mat4eyePosRight;
  ezVec2U32 m_vEyeRenderTargetSize;
};


struct ezVRDeviceType
{
  typedef ezUInt8 StorageType;
  enum Enum : ezUInt8
  {
    HMD,
    LeftController,
    RightController,
    DeviceID0,
    DeviceID1,
    DeviceID2,
    DeviceID3,
    DeviceID4,
    DeviceID5,
    DeviceID6,
    DeviceID7,
    DeviceID8,
    DeviceID9,
    DeviceID10,
    DeviceID11,
    DeviceID12,
    DeviceID13,
    DeviceID14,
    DeviceID15,
    Default = HMD,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezVRDeviceType);

/// \brief Defines the stage space used for the VR experience.
///
/// This value is set by the ezStageSpaceComponent singleton and
/// has to be taken into account by the VR implementation.
struct ezVRStageSpace
{
  typedef ezUInt8 StorageType;
  enum Enum : ezUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezVRStageSpace);

typedef ezInt8 ezVRDeviceID;
/// \brief A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the ezStageSpaceComponent singleton and
/// has to be taken into account by the VR implementation.
struct ezVRDeviceState
{
  enum class Type : ezUInt8
  {
    HMD,
    Controller,
    Tracker,
    Reference,
    Unknown,
  };

  ezMat4 m_mPose;
  ezVec3 m_vPosition;
  ezQuat m_qRotation;
  ezVec3 m_vVelocity;
  ezVec3 m_vAngularVelocity;
  Type m_Type = Type::Unknown;
  bool m_bPoseIsValid = false;
  bool m_bDeviceIsConnected = false;
};

struct ezVRDeviceEvent
{
  enum class Type : ezUInt8
  {
    DeviceAdded,
    DeviceRemoved,
  };

  Type m_Type;
  ezVRDeviceID uiDeviceID = 0;
};

class ezVRInterface
{
public:
  /// \brief Returns whether an HMD is available. Can be used to decide
  /// whether it makes sense to call Initialize at all.
  virtual bool IsHmdPresent() const = 0;

  /// \name Setup
  ///@{

  /// \brief Initializes the VR system. This can be quite time consuming
  /// as it will generally start supporting applications needed to run
  /// and start up the HMD if it went to sleep.
  virtual bool Initialize() = 0;
  /// \brief Shuts down the VR system again.
  virtual void Deinitialize() = 0;
  /// \brief Returns whether the VR system is initialized.
  virtual bool IsInitialized() const = 0;

  ///@}
  /// \name Devices
  ///@{

  /// \brief Returns general HMD information.
  virtual const ezHMDInfo& GetHmdInfo() const = 0;

  /// \brief Fills out a list of valid (connected) device IDs.
  virtual void GetDeviceList(ezHybridArray<ezVRDeviceID, 64>& out_Devices) const = 0;
  /// \brief Returns the deviceID for a specific type of device.
  /// If the device is not connected, -1 is returned instead.
  virtual ezVRDeviceID GetDeviceIDByType(ezVRDeviceType::Enum type) const = 0;
  /// \brief Returns the current device state for a valid device ID.
  virtual const ezVRDeviceState& GetDeviceState(ezVRDeviceID iDeviceID) const = 0;
  /// \brief Returns the device event.
  /// Register to it to be informed of device (dis-)connects.
  virtual ezEvent<const ezVRDeviceEvent&>& DeviceEvents() = 0;

  ///@}
  /// \name View
  ///@{

  /// \brief Creates a VR view using the given render pipeline and camera.
  /// The camera is automatically updated with the HMD transforms.
  virtual ezViewHandle CreateVRView(const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera,
    ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None) = 0;
  /// \brief Returns the VR view.
  virtual ezViewHandle GetVRView() const = 0;
  /// \brief Destroys the VR view.
  virtual bool DestroyVRView() = 0;

  /// \brief Returns true if SetCompanionViewRenderTarget is supported.
  virtual bool SupportsCompanionView() = 0;
  /// \brief Set a texture that the two eye images should be copied to.
  /// hRenderTarget must be a valid 2D texture that can be bound as a render target.
  /// Pass an invalidated handle to disable the companion view.
  virtual bool SetCompanionViewRenderTarget(ezGALTextureHandle hRenderTarget) = 0;
  virtual ezGALTextureHandle GetCompanionViewRenderTarget() const = 0;

  ///@}
};
