#pragma once

#include <System/Basics.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

#include <windows.applicationmodel.core.h>

class EZ_SYSTEM_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice(ABI::Windows::UI::Core::ICoreWindow* coreWindow);
  ~ezStandardInputDevice();

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

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

  HRESULT OnKeyEvent(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IKeyEventArgs* args);

  virtual void InitializeDevice() override;
  virtual void UpdateInputSlotValues() override {}
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  bool m_bShowCursor;
  bool m_bClipCursor;

  ABI::Windows::UI::Core::ICoreWindow* m_coreWindow;
  EventRegistrationToken m_keyDownEventRegistration;
  EventRegistrationToken m_keyUpEventRegistration;
};

