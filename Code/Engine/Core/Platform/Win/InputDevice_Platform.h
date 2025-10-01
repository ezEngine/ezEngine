#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Platform/Win/Utils/MinWindows.h>

class EZ_CORE_DLL ezInputDeviceMouseKeyboard_Win : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceMouseKeyboard_Win, ezInputDeviceMouseKeyboard);

public:
  ezInputDeviceMouseKeyboard_Win(ezMinWindows::HWND hWnd);
  ~ezInputDeviceMouseKeyboard_Win();

  /// \brief This function needs to be called by all Windows functions, to pass the input information through to this input device.
  void WindowMessage(ezMinWindows::UINT msg, ezMinWindows::WPARAM wparam, ezMinWindows::LPARAM lparam);

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
  void ApplyClipRect(ezMouseCursorClipMode::Enum mode);
  void OnFocusLost();

  ezMinWindows::HWND m_hWnd;
  static ezInputDeviceMouseKeyboard_Win* s_pGlobalInputHandler;
  bool m_bShowCursor = true;
  ezMouseCursorClipMode::Enum m_ClipCursorMode = ezMouseCursorClipMode::NoClip;
  bool m_bApplyClipRect = false;
  // m_bFirstWndMsg and m_bFirstClick are used to fix issues Windows not giving focus to applications that have been launched
  // through a parent process
  bool m_bFirstWndMsg = true;
  bool m_bFirstClick = true;
  ezUInt8 m_uiMouseButtonReceivedDown[5] = {0, 0, 0, 0, 0};
  ezUInt8 m_uiMouseButtonReceivedUp[5] = {0, 0, 0, 0, 0};
};
