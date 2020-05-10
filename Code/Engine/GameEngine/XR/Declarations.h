#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct ezXRDeviceType
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
EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMEENGINE_DLL, ezXRDeviceType);

typedef ezInt8 ezXRDeviceID;

/// \brief A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the ezStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct ezXRDeviceState
{
  ezVec3 m_vGripPosition;
  ezQuat m_qGripRotation;

  ezVec3 m_vAimPosition;
  ezQuat m_qAimRotation;

  ezEnum<ezXRDeviceType> m_Type;
  bool m_bGripPoseIsValid = false;
  bool m_bAimPoseIsValid = false;
  bool m_bDeviceIsConnected = false;
};


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

typedef ezEvent<const ezXRDeviceEventData&> ezXRDeviceEvent;
