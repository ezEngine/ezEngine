#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class EZ_CORE_DLL ezInputDeviceMouseKeyboard_NoImpl : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceMouseKeyboard_NoImpl, ezInputDeviceMouseKeyboard);

public:
  ezInputDeviceMouseKeyboard_NoImpl(ezUInt32 uiWindowNumber);
  ~ezInputDeviceMouseKeyboard_NoImpl();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
};
