#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>

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
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRStageSpace);

struct ezXRTransformSpace
{
  using StorageType = ezUInt8;
  enum Enum : ezUInt8
  {
    Local,  ///< Sets the local transform to the pose in stage space. Use if owner is direct child of ezStageSpaceComponent.
    Global, ///< Uses the global transform of the device in world space.
    Default = Local,
  };
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRTransformSpace);

struct ezXRDeviceType
{
  using StorageType = ezUInt8;
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
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRDeviceType);

using ezXRDeviceID = ezInt8;

/// \brief A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the ezStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct EZ_GAMEENGINE_DLL ezXRDeviceState
{
  ezXRDeviceState();

  ezVec3 m_vGripPosition;
  ezQuat m_qGripRotation;

  ezVec3 m_vAimPosition;
  ezQuat m_qAimRotation;

  ezEnum<ezXRDeviceType> m_Type;
  bool m_bGripPoseIsValid = false;
  bool m_bAimPoseIsValid = false;
  bool m_bDeviceIsConnected = false;
};

/// \brief Defines features the given device supports.
struct ezXRDeviceFeatures
{
  using StorageType = ezUInt32;
  enum Enum : ezUInt32
  {
    None = 0,
    Trigger = EZ_BIT(0),                   ///< Float input. If fully pressed, will also trigger 'Select'.
    Select = EZ_BIT(1),                    ///< Bool input.
    Menu = EZ_BIT(2),                      ///< Bool input.
    Squeeze = EZ_BIT(3),                   ///< Bool input.
    PrimaryAnalogStick = EZ_BIT(4),        ///< 2D axis input.
    PrimaryAnalogStickClick = EZ_BIT(5),   ///< Bool input.
    PrimaryAnalogStickTouch = EZ_BIT(6),   ///< Bool input.
    SecondaryAnalogStick = EZ_BIT(7),      ///< 2D axis input.
    SecondaryAnalogStickClick = EZ_BIT(8), ///< Bool input.
    SecondaryAnalogStickTouch = EZ_BIT(9), ///< Bool input.
    GripPose = EZ_BIT(10),                 ///< 3D Pose input.
    AimPose = EZ_BIT(11),                  ///< 3D Pose input.
    Default = None
  };

  struct Bits
  {
    StorageType Trigger : 1;
    StorageType Select : 1;
    StorageType Menu : 1;
    StorageType Squeeze : 1;
    StorageType PrimaryAnalogStick : 1;
    StorageType PrimaryAnalogStickClick : 1;
    StorageType PrimaryAnalogStickTouch : 1;
    StorageType SecondaryAnalogStick : 1;
    StorageType SecondaryAnalogStickClick : 1;
    StorageType SecondaryAnalogStickTouch : 1;
    StorageType GripPose : 1;
    StorageType AimPose : 1;
  };
};
EZ_DECLARE_FLAGS_OPERATORS(ezXRDeviceFeatures);


struct ezXRDeviceEventData
{
  enum class Type : ezUInt8
  {
    DeviceAdded,
    DeviceRemoved,
  };

  Type m_Type;
  ezXRDeviceID uiDeviceID = 0;
};

using ezXRDeviceEvent = ezEvent<const ezXRDeviceEventData&>;
