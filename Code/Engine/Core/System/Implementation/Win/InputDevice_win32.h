#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Basics/Platform/Win/MinWindows.h>

class EZ_CORE_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice(ezUInt32 uiWindowNumber);
  ~ezStandardInputDevice();

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(ezMinWindows::HWND hWnd, ezMinWindows::UINT Msg, ezMinWindows::WPARAM wParam, ezMinWindows::LPARAM lParam);

  /// \brief Calling this function will 'translate' most key names from English to the OS language, by querying that information
  /// from the OS.
  ///
  /// The OS translation might not always be perfect for all keys. The translation can change when the user changes the keyboard layout.
  /// So if he switches from an English layout to a German layout, LocalizeButtonDisplayNames() should be called again, to update
  /// the display names, if that is required.
  static void LocalizeButtonDisplayNames();

  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

protected:
  virtual void InitializeDevice() override;
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;
  virtual void UpdateInputSlotValues() override;

private:
  void ApplyClipRect(ezMouseCursorClipMode::Enum mode, ezMinWindows::HWND hWnd);
  void OnFocusLost(ezMinWindows::HWND hWnd);

  static bool s_bMainWindowUsed;
  ezUInt32 m_uiWindowNumber = 0;
  bool m_bShowCursor = true;
  ezMouseCursorClipMode::Enum m_ClipCursorMode = ezMouseCursorClipMode::NoClip;
  bool m_bApplyClipRect = false;
  ezUInt8 m_uiMouseButtonReceivedDown[5];
  ezUInt8 m_uiMouseButtonReceivedUp[5];
};
