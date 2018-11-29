#pragma once
#include <RendererFoundation/Basics.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;
class ezViewHandle;
class ezCamera;

struct ezHMDInfo
{
  ezString m_sDeviceName;
  ezString m_sDeviceDriver;
  ezMat4 m_mat4eyePosLeft;
  ezMat4 m_mat4eyePosRight;
  ezVec2U32 m_vEyeRenderTargetSize;
  ezUInt8 uiDeviceID = 0;
};

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
  ezUInt8 uiDeviceID = 0;
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
  virtual void GetDeviceList(ezHybridArray<ezUInt8, 64>& out_Devices) const = 0;
  /// \brief Returns the current device state for a valid device ID.
  virtual const ezVRDeviceState& GetDeviceState(ezUInt8 uiDeviceID) const = 0;
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

  ///@}
};
