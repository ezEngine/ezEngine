#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class EZ_CORE_DLL ezInputDeviceMouseKeyboard_NoImpl : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceMouseKeyboard_NoImpl, ezInputDeviceMouseKeyboard);

public:
  ezInputDeviceMouseKeyboard_NoImpl(ezUInt32 uiWindowNumber);
  ~ezInputDeviceMouseKeyboard_NoImpl();

private:
  virtual void ApplyShowMouseCursor(bool bShow, bool bCustomCursorActive) override;
  virtual void ApplyClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;

  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};
