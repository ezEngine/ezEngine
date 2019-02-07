#include <GameEnginePCH.h>

#include <GameEngine/Interfaces/VRInterface.h>
#include <Foundation/Reflection/Reflection.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVRDeviceType, 1)
EZ_BITFLAGS_CONSTANTS(ezVRDeviceType::HMD, ezVRDeviceType::LeftController, ezVRDeviceType::RightController)
EZ_BITFLAGS_CONSTANTS(ezVRDeviceType::DeviceID0, ezVRDeviceType::DeviceID1, ezVRDeviceType::DeviceID2, ezVRDeviceType::DeviceID3)
EZ_BITFLAGS_CONSTANTS(ezVRDeviceType::DeviceID4, ezVRDeviceType::DeviceID5, ezVRDeviceType::DeviceID6, ezVRDeviceType::DeviceID7)
EZ_BITFLAGS_CONSTANTS(ezVRDeviceType::DeviceID8, ezVRDeviceType::DeviceID9, ezVRDeviceType::DeviceID10, ezVRDeviceType::DeviceID11)
EZ_BITFLAGS_CONSTANTS(ezVRDeviceType::DeviceID12, ezVRDeviceType::DeviceID13, ezVRDeviceType::DeviceID14, ezVRDeviceType::DeviceID15)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezVRStageSpace, 1)
EZ_BITFLAGS_CONSTANTS(ezVRStageSpace::Seated, ezVRStageSpace::Standing)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on


EZ_STATICLINK_FILE(GameEngine, GameEngine_Interfaces_VRInterface);

