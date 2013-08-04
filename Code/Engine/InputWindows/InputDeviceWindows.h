#pragma once

#include <InputWindows/Basics.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

class EZ_INPUTWINDOWS_DLL ezInputDeviceWindows : public ezInputDeviceMouseKeyboard
{
public:
  ezInputDeviceWindows(ezUInt32 uiWindowNumber);
  ~ezInputDeviceWindows();

  static ezInputDeviceWindows* GetDevice(ezUInt32 uiWindowNumber = 0);
  static void DestroyAllDevices();

  virtual const char* GetDeviceName() const { return "MouseKeyboardWindows"; }

  void WindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

  static void LocalizeButtonDisplayNames();

  void SetClipMouseCursor(bool bEnable);
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

