#pragma once

#include <InputXBox360/Basics.h>
#include <Core/Input/DeviceTypes/Controller.h>

class EZ_INPUTXBOX360_DLL ezInputDeviceXBox360 : public ezInputDeviceController
{
public:
  ezInputDeviceXBox360();

  static ezInputDeviceXBox360* GetDevice();
  static void DestroyAllDevices();

  virtual const char* GetDeviceName() const { return "ControllerXBox"; }

  virtual bool IsControllerConnected(ezUInt8 uiPhysical) const EZ_OVERRIDE { EZ_ASSERT(uiPhysical < 4, "Invalid Controller Index"); return m_bControllerConnected[uiPhysical]; }

  virtual void EnableVibration(ezUInt8 uiController, bool bEnable) EZ_OVERRIDE { m_bEnableVibration[uiController] = bEnable; }
  virtual bool IsVibrationEnabled(ezUInt8 uiController) const EZ_OVERRIDE { return m_bEnableVibration[uiController]; }

  virtual void SetPhysicalToVirtualControllerMapping(ezUInt8 uiPhysical, ezInt8 iVirtual) EZ_OVERRIDE { m_iMapControllerTo[uiPhysical] = iVirtual; }
  virtual ezInt8 GetPhysicalToVirtualControllerMapping(ezUInt8 uiPhysical) const EZ_OVERRIDE { return m_iMapControllerTo[uiPhysical]; }

private:
  bool m_bControllerConnected[4];
  ezInt8 m_iMapControllerTo[4];
  bool m_bEnableVibration[4];

  virtual void InitializeDevice() EZ_OVERRIDE { }
  virtual void UpdateInputSlotValues() EZ_OVERRIDE;
  virtual void RegisterInputSlots() EZ_OVERRIDE;

  void SetValue(ezInt32 iController, const char* szButton, float fValue);

  static void RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags);
  static void SetDeadZoneAndScale(const char* szButton);
};

