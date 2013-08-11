#pragma once

#include <InputXBox360/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>

/// \brief An implementation of ezInputDeviceController that handles XBox 360 controllers.
///
/// Works on all platforms that provide the XINPUT API.
class EZ_INPUTXBOX360_DLL ezInputDeviceXBox360 : public ezInputDeviceController
{
public:
  ezInputDeviceXBox360();

  /// \brief Returns an ezInputDeviceXBox360 device.
  static ezInputDeviceXBox360* GetDevice();

  /// \brief Destroys all devices of this type. Automatically called at engine shutdown.
  static void DestroyAllDevices();

  /// \brief Returns "ControllerXBox"
  virtual const char* GetDeviceName() const { return "ControllerXBox"; }

  virtual bool IsControllerConnected(ezUInt8 uiPhysical) const EZ_OVERRIDE;

private:
  virtual void ApplyVibration(ezUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) EZ_OVERRIDE;

  bool m_bControllerConnected[4];

  virtual void InitializeDevice() EZ_OVERRIDE { }
  virtual void UpdateInputSlotValues(double fTimeDifference) EZ_OVERRIDE;
  virtual void RegisterInputSlots() EZ_OVERRIDE;

  void SetValue(ezInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);
};

