#include <GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRTransformSpace, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRTransformSpace::Local, ezXRTransformSpace::Global)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezXRDeviceType, 1)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::HMD, ezXRDeviceType::LeftController, ezXRDeviceType::RightController)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID0, ezXRDeviceType::DeviceID1, ezXRDeviceType::DeviceID2, ezXRDeviceType::DeviceID3)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID4, ezXRDeviceType::DeviceID5, ezXRDeviceType::DeviceID6, ezXRDeviceType::DeviceID7)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID8, ezXRDeviceType::DeviceID9, ezXRDeviceType::DeviceID10, ezXRDeviceType::DeviceID11)
  EZ_BITFLAGS_CONSTANTS(ezXRDeviceType::DeviceID12, ezXRDeviceType::DeviceID13, ezXRDeviceType::DeviceID14, ezXRDeviceType::DeviceID15)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezXRDeviceState::ezXRDeviceState()
{
  m_vGripPosition.SetZero();
  m_qGripRotation.SetIdentity();

  m_vAimPosition.SetZero();
  m_qAimRotation.SetIdentity();
}
