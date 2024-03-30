#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>

struct ezAndroidInputEvent;
struct AInputEvent;

/// \brief Android standard input device.
class EZ_CORE_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice(ezUInt32 uiWindowNumber);
  ~ezStandardInputDevice();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

private:
  void AndroidInputEventHandler(ezAndroidInputEvent& event);
  ezInt32 AndroidHandleInput(AInputEvent* pEvent);
};
