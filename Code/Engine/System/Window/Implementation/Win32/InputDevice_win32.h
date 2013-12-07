#pragma once

#include <System/Basics.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class EZ_SYSTEM_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
public:
  ezStandardInputDevice(ezUInt32 uiWindowNumber);
  ~ezStandardInputDevice();

  /// \brief Returns "MouseKeyboardWindows".
  virtual const char* GetDeviceName() const EZ_OVERRIDE { return "MouseKeyboardWindows"; }

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from and English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  /// \brief Will trap the mouse inside the application window. Should usually be enabled, to prevent accidental task switches.
  ///
  /// Especially on multi-monitor systems, the mouse can easily leave the application window (even in fullscreen mode).
  /// Do NOT use this function when you have multiple windows and require absolute mouse positions.
  void SetClipMouseCursor(bool bEnable);

  /// \brief Returns whether the mouse is confined to the application window or not.
  bool GetClipMouseCursor() const { return m_bClipCursor; }

  virtual void SetShowMouseCursor(bool bShow) EZ_OVERRIDE;
  virtual bool GetShowMouseCursor() const EZ_OVERRIDE;

private:
  virtual void InitializeDevice() EZ_OVERRIDE;
  virtual void UpdateInputSlotValues() EZ_OVERRIDE { }
  virtual void RegisterInputSlots() EZ_OVERRIDE;
  virtual void ResetInputSlotValues() EZ_OVERRIDE;

  static bool s_bMainWindowUsed;
  ezUInt32 m_uiWindowNumber;
  bool m_bShowCursor;
  bool m_bClipCursor;
};

