#pragma once

#include <Core/Input/DeviceTypes/Controller.h>
#include <GameEngine/GameEngineDLL.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

/// \brief An implementation of ezInputDeviceController that handles XBox controllers.
///
/// Works on all platforms that provide the XINPUT API.
class EZ_GAMEENGINE_DLL ezInputDeviceXBoxController : public ezInputDeviceController
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceXBoxController, ezInputDeviceController);

public:
  ezInputDeviceXBoxController();
  ~ezInputDeviceXBoxController();

  /// \brief Returns an ezInputDeviceXBoxController device.
  static ezInputDeviceXBoxController* GetDevice();
  virtual bool IsPhysicalControllerConnected(ezUInt8 uiPhysical) const override;

  /// Maps connected controllers to virtual controllers in the order of which ones are connected.
  ///
  /// So usually 0->0, 1->1, etc.
  /// But if for instance controller 0 is not connected, it would be 1->0, 2->1, 3->2, 0->3
  ///
  /// By default all controllers map to virtual controller 0, so 0->0, 1->0, 2->0, 3->0.
  void SetupControllerMappingInOrder();

private:
  static void RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags);
  static void SetDeadZone(const char* szButton);

  virtual void ApplyVibration(ezUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) override;
  virtual void InitializeDevice() override {}
  virtual void UpdateInputSlotValues() override;
  virtual void RegisterInputSlots() override;
  virtual void UpdateHardwareState(ezTime tTimeDifference) override;

  void SetValue(ezInt32 iController, const char* szButton, float fValue);

  bool m_bControllerConnected[MaxControllers];
};

#endif
