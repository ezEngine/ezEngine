#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WEB)

#  include <Core/Input/DeviceTypes/MouseKeyboard.h>
#  include <Foundation/Containers/ArrayMap.h>

class EZ_CORE_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice();
  ~ezStandardInputDevice();

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;
  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override;

public:
  // Web callbacks to set the state
  static void onWebChar(const std::string& text);
  static void onWebKey(const std::string& scancode, bool bDown);
  static void onWebMouseClick(ezInt32 iButton, bool bDown);
  static void onWebMouseMove(double x, double y);
  static void onWebMouseLeave();
  static void onWebMouseWheel(double y);

private:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  static ezArrayMap<ezUInt32, ezStringView> s_WebKeyNameToInputSlot;

  ezVec2d m_LastPos = ezVec2d(ezMath::MaxValue<double>());
};

#endif
