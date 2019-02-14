#pragma once

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <System/SystemDLL.h>

#include <windows.applicationmodel.core.h>
#include <wrl/client.h>

class EZ_SYSTEM_DLL ezStandardInputDevice : public ezInputDeviceMouseKeyboard
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStandardInputDevice, ezInputDeviceMouseKeyboard);

public:
  ezStandardInputDevice(ABI::Windows::UI::Core::ICoreWindow* coreWindow);
  ~ezStandardInputDevice();

  virtual void SetClipMouseCursor(ezMouseCursorClipMode::Enum mode) override;
  virtual ezMouseCursorClipMode::Enum GetClipMouseCursor() const override { return m_ClipCursorMode; }

  virtual void SetShowMouseCursor(bool bShow) override;
  virtual bool GetShowMouseCursor() const override;

private:
  HRESULT OnKeyEvent(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IKeyEventArgs* args);
  HRESULT OnCharacterReceived(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::ICharacterReceivedEventArgs* args);
  HRESULT OnPointerMovePressEnter(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerWheelChange(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerReleasedOrExited(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnPointerCaptureLost(ABI::Windows::UI::Core::ICoreWindow* coreWindow, ABI::Windows::UI::Core::IPointerEventArgs* args);
  HRESULT OnMouseMoved(ABI::Windows::Devices::Input::IMouseDevice* mouseDevice, ABI::Windows::Devices::Input::IMouseEventArgs* args);



  HRESULT UpdateMouseButtonStates(ABI::Windows::UI::Input::IPointerPoint* pointerPoint);

  virtual void InitializeDevice() override;
  virtual void UpdateInputSlotValues() override {}
  virtual void RegisterInputSlots() override;
  virtual void ResetInputSlotValues() override;

  bool m_bShowCursor = true;
  ezMouseCursorClipMode::Enum m_ClipCursorMode = ezMouseCursorClipMode::NoClip;


  Microsoft::WRL::ComPtr<ABI::Windows::UI::Core::ICoreWindow> m_coreWindow;
  Microsoft::WRL::ComPtr<ABI::Windows::UI::Core::ICoreCursor> m_cursorBeforeHide;
  Microsoft::WRL::ComPtr<ABI::Windows::Devices::Input::IMouseDevice> m_mouseDevice;

  EventRegistrationToken m_eventRegistration_keyDown;
  EventRegistrationToken m_eventRegistration_keyUp;
  EventRegistrationToken m_eventRegistration_characterReceived;
  EventRegistrationToken m_eventRegistration_pointerMoved;
  EventRegistrationToken m_eventRegistration_pointerEntered;
  EventRegistrationToken m_eventRegistration_pointerExited;
  EventRegistrationToken m_eventRegistration_pointerCaptureLost;
  EventRegistrationToken m_eventRegistration_pointerPressed;
  EventRegistrationToken m_eventRegistration_pointerReleased;
  EventRegistrationToken m_eventRegistration_pointerWheelChanged;
  EventRegistrationToken m_eventRegistration_mouseMoved;
};

