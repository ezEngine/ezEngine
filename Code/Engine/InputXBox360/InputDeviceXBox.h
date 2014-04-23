#pragma once

#include <InputXBox360/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>

/// \brief An implementation of ezInputDeviceController that handles XBox 360 controllers.
///
/// Works on all platforms that provide the XINPUT API.
class EZ_INPUTXBOX360_DLL ezInputDeviceXBox360 : public ezInputDeviceController
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceXBox360);

public:
  ezInputDeviceXBox360();

  /// \brief Returns an ezInputDeviceXBox360 device.
  static ezInputDeviceXBox360* GetDevice();

  /// \brief Destroys all devices of this type. Automatically called at engine shutdown.
  static void DestroyAllDevices();

  virtual bool IsControllerConnected(ezUInt8 uiPhysical) const override;

private:
  virtual void ApplyVibration(ezUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;

  bool m_bControllerConnected[4];

  virtual void InitializeDevice() override { }
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(ezTime tTimeDifference) override;

  void SetValue(ezInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);
};

