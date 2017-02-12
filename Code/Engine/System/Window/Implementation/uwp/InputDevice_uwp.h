#pragma once

#include <System/Basics.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class EZ_SYSTEM_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice();
  ~ezStandardInputDevice();

  /// \brief Will trap the mouse inside the application window. Should usually be enabled, to prevent accidental task switches.
  ///
  /// Especially on multi-monitor systems, the mouse can easily leave the application window (even in fullscreen mode).
  /// Do NOT use this function when you have multiple windows and require absolute mouse positions.
  void SetClipMouseCursor(bool bEnable);

  /// \brief Returns whether the mouse is confined to the application window or not.
  bool GetClipMouseCursor() const { return m_bClipCursor; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

private:
  virtual void InitializeDevice() override;
  virtual void UpdateInputSlotValues() override {}
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  bool m_bShowCursor;
  bool m_bClipCursor;
};

