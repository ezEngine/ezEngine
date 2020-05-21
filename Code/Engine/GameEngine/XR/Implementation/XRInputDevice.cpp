#include <GameEnginePCH.h>

#include <GameEngine/XR/XRInputDevice.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRDeviceType, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::HMD, ezXRDeviceType::LeftController, ezXRDeviceType::RightController)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID0, ezXRDeviceType::DeviceID1, ezXRDeviceType::DeviceID2, ezXRDeviceType::DeviceID3)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID4, ezXRDeviceType::DeviceID5, ezXRDeviceType::DeviceID6, ezXRDeviceType::DeviceID7)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID8, ezXRDeviceType::DeviceID9, ezXRDeviceType::DeviceID10, ezXRDeviceType::DeviceID11)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID12, ezXRDeviceType::DeviceID13, ezXRDeviceType::DeviceID14, ezXRDeviceType::DeviceID15)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezXRInputDevice, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRInputDevice);
