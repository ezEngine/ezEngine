#include <PCH.h>
#include <Foundation/Logging/Log.h>
#include <System/Window/Implementation/uwp/InputDevice_uwp.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Containers/HybridArray.h>


#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <wrl/event.h>

using namespace ABI::Windows::UI::Core;


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

ezStandardInputDevice::ezStandardInputDevice(ICoreWindow* coreWindow) : m_coreWindow(coreWindow)
{
  // TODO
  m_bClipCursor = false;
  m_bShowCursor = true;
}

ezStandardInputDevice::~ezStandardInputDevice()
{
  if (m_coreWindow)
  {
    m_coreWindow->remove_KeyDown(m_eventRegistration_keyDown);
    m_coreWindow->remove_KeyUp(m_eventRegistration_keyUp);
    m_coreWindow->remove_CharacterReceived(m_eventRegistration_characterReceived);
    m_coreWindow->remove_PointerMoved(m_eventRegistration_pointerMoved);
    m_coreWindow->remove_PointerEntered(m_eventRegistration_pointerEntered);
    m_coreWindow->remove_PointerExited(m_eventRegistration_pointerExited);
    m_coreWindow->remove_PointerCaptureLost(m_eventRegistration_pointerCaptureLost);
    m_coreWindow->remove_PointerPressed(m_eventRegistration_pointerPressed);
    m_coreWindow->remove_PointerReleased(m_eventRegistration_pointerReleased);
    m_coreWindow->remove_PointerWheelChanged(m_eventRegistration_pointerWheelChanged);
  }

  if (m_mouseDevice)
  {
    m_mouseDevice->remove_MouseMoved(m_eventRegistration_mouseMoved);
  }
}

void ezStandardInputDevice::InitializeDevice()
{
  using KeyHandler = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs;
  using CharacterReceivedHandler = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CCharacterReceivedEventArgs;
  using PointerHander = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CPointerEventArgs;

  // Keyboard
  m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &ezStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyDown);
  m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &ezStandardInputDevice::OnKeyEvent).Get(), &m_eventRegistration_keyUp);
  m_coreWindow->add_CharacterReceived(Callback<CharacterReceivedHandler>(this, &ezStandardInputDevice::OnCharacterReceived).Get(), &m_eventRegistration_characterReceived);

  // Pointer
  // Note that a pointer may be mouse, pen/stylus or touch!
  // We bundle move/press/enter all in a single callback to update all pointer state - all these cases have in common that pen/touch is pressed now.
  m_coreWindow->add_PointerMoved(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerMoved);
  m_coreWindow->add_PointerEntered(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerEntered);
  m_coreWindow->add_PointerPressed(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerMovePressEnter).Get(), &m_eventRegistration_pointerPressed);
  // Changes in the pointer wheel:
  m_coreWindow->add_PointerWheelChanged(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerWheelChange).Get(), &m_eventRegistration_pointerWheelChanged);
  // Exit for touch or stylus means that we no longer have a press.
  // However, we presserve mouse button presses.
  m_coreWindow->add_PointerExited(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerReleasedOrExited).Get(), &m_eventRegistration_pointerExited);
  m_coreWindow->add_PointerReleased(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerReleasedOrExited).Get(), &m_eventRegistration_pointerReleased);
  // Capture loss.
  // From documentation "Occurs when a pointer moves to another app. This event is raised after PointerExited and is the final event received by the app for this pointer."
  // If this happens we want to release all mouse buttons as well.
  m_coreWindow->add_PointerCaptureLost(Callback<PointerHander>(this, &ezStandardInputDevice::OnPointerCaptureLost).Get(), &m_eventRegistration_pointerCaptureLost);


  // Mouse
  // The only thing that we get from the MouseDevice class is mouse moved which gives us unfiltered relative mouse position.
  // Everything else is done by WinRt's "Pointer"
  // https://docs.microsoft.com/en-us/uwp/api/windows.devices.input.mousedevice
  // Relevant article for mouse move:
  // https://docs.microsoft.com/en-us/windows/uwp/gaming/relative-mouse-movement
  {
    ComPtr<ABI::Windows::Devices::Input::IMouseDeviceStatics> mouseDeviceStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Devices_Input_MouseDevice).Get(), &mouseDeviceStatics)))
    {
      if (SUCCEEDED(mouseDeviceStatics->GetForCurrentView(&m_mouseDevice)))
      {
        using MouseMovedHandler = __FITypedEventHandler_2_Windows__CDevices__CInput__CMouseDevice_Windows__CDevices__CInput__CMouseEventArgs;
        m_mouseDevice->add_MouseMoved(Callback<MouseMovedHandler>(this, &ezStandardInputDevice::OnMouseMoved).Get(), &m_eventRegistration_mouseMoved);
      }
    }
  }
}

HRESULT ezStandardInputDevice::OnKeyEvent(ICoreWindow* coreWindow, IKeyEventArgs* args)
{
  // Closely related to the RawInput implementation in Win32/InputDevice_win32.inl

  CorePhysicalKeyStatus keyStatus;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_KeyStatus(&keyStatus));

  static bool bWasStupidLeftShift = false;

  if (keyStatus.ScanCode == 42 && keyStatus.IsExtendedKey) // 42 has to be special I guess
  {
    bWasStupidLeftShift = true;
    return S_OK;
  }

  const char* szInputSlotName = ezInputManager::ConvertScanCodeToEngineName(keyStatus.ScanCode, keyStatus.IsExtendedKey == TRUE);
  if (!szInputSlotName)
    return S_OK;


  // Don't know yet how to handle this in UWP:

  // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
  // so we need to fix this manually
  //if (raw->data.keyboard.Flags & RI_KEY_E1)
  //{
  //  szInputSlotName = ezInputSlot_KeyPause;
  //  bIgnoreNext = true;
  //}


  // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
  // we ignore the first stupid shift key entirely and then modify the following Numpad* key
  // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
  // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
  if (ezStringUtils::IsEqual(szInputSlotName, ezInputSlot_KeyNumpadStar) && bWasStupidLeftShift)
    szInputSlotName = ezInputSlot_KeyPrint;

  bWasStupidLeftShift = false;

  m_InputSlotValues[szInputSlotName] = keyStatus.IsKeyReleased ? 0.0f : 1.0f;

  return S_OK;
}

HRESULT ezStandardInputDevice::OnCharacterReceived(ICoreWindow* coreWindow, ICharacterReceivedEventArgs* args)
{
  UINT32 keyCode = 0;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_KeyCode(&keyCode));
  m_LastCharacter = keyCode;

  return S_OK;
}

HRESULT ezStandardInputDevice::OnPointerMovePressEnter(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerDevice->get_PointerDeviceType(&deviceType));

  // Pointer position.
  // From the documention: "The position of the pointer in device-independent pixel (DIP)."
  // Note also, that there is "raw position" which may be free of pointer prediction etc.
  ABI::Windows::Foundation::Point pointerPosition;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_Position(&pointerPosition));
  ABI::Windows::Foundation::Rect windowRectangle;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(coreWindow->get_Bounds(&windowRectangle)); // Bounds are in DIP as well!
  
  float relativePosX = static_cast<float>(pointerPosition.X) / windowRectangle.Width;
  float relativePosY = static_cast<float>(pointerPosition.Y) / windowRectangle.Height;

  if (deviceType == PointerDeviceType_Mouse)
  {
    // TODO
    //RegisterInputSlot(ezInputSlot_MouseDblClick0, "Left Double Click", ezInputSlotFlags::IsDoubleClick);
    //RegisterInputSlot(ezInputSlot_MouseDblClick1, "Right Double Click", ezInputSlotFlags::IsDoubleClick);
    //RegisterInputSlot(ezInputSlot_MouseDblClick2, "Middle Double Click", ezInputSlotFlags::IsDoubleClick);

    m_InputSlotValues[ezInputSlot_MousePositionX] = relativePosX;
    m_InputSlotValues[ezInputSlot_MousePositionY] = relativePosY;

    EZ_SUCCEED_OR_PASS_HRESULT_ON(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    // All callbacks we subscribed this event to imply that a touch occurs right now.
    m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(pointerId)] = 1.0f;  // Touch strength?
    m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionX(pointerId)] = relativePosX;
    m_InputSlotValues[ezInputManager::GetInputSlotTouchPointPositionY(pointerId)] = relativePosY;
  }

  return S_OK;
}

HRESULT ezStandardInputDevice::OnPointerWheelChange(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerDevice(&pointerDevice));

  // Only interested in mouse devices.
  PointerDeviceType deviceType;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerDevice->get_PointerDeviceType(&deviceType));
  if (deviceType == PointerDeviceType_Mouse)
  {
    ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
    EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_Properties(&properties));

    // .. and only vertical wheels.
    boolean isHorizontalWheel;
    EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsHorizontalMouseWheel(&isHorizontalWheel));
    if (!isHorizontalWheel)
    {
      INT32 delta;
      EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_MouseWheelDelta(&delta));

      if (delta > 0)
        m_InputSlotValues[ezInputSlot_MouseWheelUp] = delta / 120.0f;
      else
        m_InputSlotValues[ezInputSlot_MouseWheelDown] = -delta / 120.0f;
    }
  }

  return S_OK;
}

HRESULT ezStandardInputDevice::OnPointerReleasedOrExited(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    // Note that the relased event is only fired if the last mouse button is released according to documentation.
    // However, we're also subscribing to exit and depending on the mouse capture this may or may not be a button release.
    EZ_SUCCEED_OR_PASS_HRESULT_ON(UpdateMouseButtonStates(pointerPoint.Get()));
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT ezStandardInputDevice::OnPointerCaptureLost(ICoreWindow* coreWindow, IPointerEventArgs* args)
{
  using namespace ABI::Windows::Devices::Input;

  ComPtr<ABI::Windows::UI::Input::IPointerPoint> pointerPoint;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_CurrentPoint(&pointerPoint));
  ComPtr<IPointerDevice> pointerDevice;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerDevice(&pointerDevice));
  PointerDeviceType deviceType;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerDevice->get_PointerDeviceType(&deviceType));

  if (deviceType == PointerDeviceType_Mouse)
  {
    m_InputSlotValues[ezInputSlot_MouseButton0] = 0.0f;
    m_InputSlotValues[ezInputSlot_MouseButton1] = 0.0f;
    m_InputSlotValues[ezInputSlot_MouseButton2] = 0.0f;
    m_InputSlotValues[ezInputSlot_MouseButton3] = 0.0f;
    m_InputSlotValues[ezInputSlot_MouseButton4] = 0.0f;
  }
  else // Touch AND Pen
  {
    // WinRT treats each touch point as unique pointer.
    UINT32 pointerId;
    EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_PointerId(&pointerId));
    if (pointerId > 9)
      return S_OK;

    m_InputSlotValues[ezInputManager::GetInputSlotTouchPoint(pointerId)] = 0.0f;
  }

  return S_OK;
}

HRESULT ezStandardInputDevice::OnMouseMoved(ABI::Windows::Devices::Input::IMouseDevice* mouseDevice, ABI::Windows::Devices::Input::IMouseEventArgs* args)
{
  ABI::Windows::Devices::Input::MouseDelta mouseDelta;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(args->get_MouseDelta(&mouseDelta));

  m_InputSlotValues[ezInputSlot_MouseMoveNegX] += ((mouseDelta.X < 0) ? static_cast<float>(-mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[ezInputSlot_MouseMovePosX] += ((mouseDelta.X > 0) ? static_cast<float>(mouseDelta.X) : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[ezInputSlot_MouseMoveNegY] += ((mouseDelta.Y < 0) ? static_cast<float>(-mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;
  m_InputSlotValues[ezInputSlot_MouseMovePosY] += ((mouseDelta.Y > 0) ? static_cast<float>(mouseDelta.Y) : 0.0f) * GetMouseSpeed().y;

  return S_OK;
}

HRESULT ezStandardInputDevice::UpdateMouseButtonStates(ABI::Windows::UI::Input::IPointerPoint* pointerPoint)
{
  ComPtr<ABI::Windows::UI::Input::IPointerPointProperties> properties;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(pointerPoint->get_Properties(&properties));

  boolean isPressed;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsLeftButtonPressed(&isPressed));
  m_InputSlotValues[ezInputSlot_MouseButton0] = isPressed ? 1.0f : 0.0f;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsRightButtonPressed(&isPressed));
  m_InputSlotValues[ezInputSlot_MouseButton1] = isPressed ? 1.0f : 0.0f;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsMiddleButtonPressed(&isPressed));
  m_InputSlotValues[ezInputSlot_MouseButton2] = isPressed ? 1.0f : 0.0f;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsXButton1Pressed(&isPressed));
  m_InputSlotValues[ezInputSlot_MouseButton3] = isPressed ? 1.0f : 0.0f;
  EZ_SUCCEED_OR_PASS_HRESULT_ON(properties->get_IsXButton2Pressed(&isPressed));
  m_InputSlotValues[ezInputSlot_MouseButton4] = isPressed ? 1.0f : 0.0f;

  return S_OK;
}

void ezStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(ezInputSlot_KeyLeft, "Left", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRight, "Right", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyUp, "Up", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyDown, "Down", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyEscape, "Escape", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeySpace, "Space", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBackspace, "Backspace", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyReturn, "Return", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyTab, "Tab", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftShift, "Left Shift", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightShift, "Right Shift", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftCtrl, "Left Ctrl", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightCtrl, "Right Ctrl", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftAlt, "Left Alt", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightAlt, "Right Alt", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftWin, "Left Win", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightWin, "Right Win", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyBracketOpen, "[", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBracketClose, "]", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeySemicolon, ";", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyApostrophe, "'", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeySlash, "/", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyEquals, "=", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyTilde, "~", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyHyphen, "-", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyComma, ",", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPeriod, ".", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBackslash, "\\", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPipe, "|", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_Key1, "1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key2, "2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key3, "3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key4, "4", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key5, "5", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key6, "6", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key7, "7", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key8, "8", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key9, "9", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_Key0, "0", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyNumpad1, "Numpad 1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad2, "Numpad 2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad3, "Numpad 3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad4, "Numpad 4", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad5, "Numpad 5", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad6, "Numpad 6", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad7, "Numpad 7", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad8, "Numpad 8", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad9, "Numpad 9", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpad0, "Numpad 0", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyA, "A", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyB, "B", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyC, "C", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyD, "D", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyE, "E", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF, "F", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyG, "G", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyH, "H", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyI, "I", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyJ, "J", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyK, "K", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyL, "L", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyM, "M", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyN, "N", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyO, "O", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyP, "P", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyQ, "Q", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyR, "R", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyS, "S", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyT, "T", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyU, "U", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyV, "V", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyW, "W", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyX, "X", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyY, "Y", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyZ, "Z", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyF1, "F1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF2, "F2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF3, "F3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF4, "F4", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF5, "F5", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF6, "F6", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF7, "F7", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF8, "F8", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF9, "F9", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF10, "F10", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF11, "F11", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF12, "F12", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyHome, "Home", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyEnd, "End", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyDelete, "Delete", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyInsert, "Insert", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPageUp, "Page Up", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPageDown, "Page Down", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyNumLock, "Numlock", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadPlus, "Numpad +", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadMinus, "Numpad -", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadStar, "Numpad *", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadSlash, "Numpad /", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadPeriod, "Numpad .", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadEnter, "Enter", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyCapsLock, "Capslock", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPrint, "Print", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyScroll, "Scroll", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPause, "Pause", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyApps, "Application", ezInputSlotFlags::IsButton);

  //RegisterInputSlot(ezInputSlot_KeyPrevTrack, "Previous Track", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyNextTrack, "Next Track", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyPlayPause, "Play / Pause", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyStop, "Stop", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyVolumeUp, "Volume Up", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyVolumeDown, "Volume Down", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyMute, "Mute", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_MouseWheelUp, "Mousewheel Up", ezInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(ezInputSlot_MouseWheelDown, "Mousewheel Down", ezInputSlotFlags::IsMouseWheel);

  RegisterInputSlot(ezInputSlot_MouseMoveNegX, "Mouse Move Left",   ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosX, "Mouse Move Right",  ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMoveNegY, "Mouse Move Down",   ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosY, "Mouse Move Up",     ezInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(ezInputSlot_MouseButton0, "Mousebutton 0", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton1, "Mousebutton 1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton2, "Mousebutton 2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton3, "Mousebutton 3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton4, "Mousebutton 4", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_MouseDblClick0, "Left Double Click",   ezInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(ezInputSlot_MouseDblClick1, "Right Double Click",  ezInputSlotFlags::IsDoubleClick);
  RegisterInputSlot(ezInputSlot_MouseDblClick2, "Middle Double Click", ezInputSlotFlags::IsDoubleClick);

  RegisterInputSlot(ezInputSlot_MousePositionX, "Mouse Position X", ezInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(ezInputSlot_MousePositionY, "Mouse Position Y", ezInputSlotFlags::IsMouseAxisPosition);


  // Not yet supported
  RegisterInputSlot(ezInputSlot_TouchPoint0, "Touchpoint 1", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint0_PositionX, "Touchpoint 1 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint0_PositionY, "Touchpoint 1 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint1, "Touchpoint 2", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint1_PositionX, "Touchpoint 2 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint1_PositionY, "Touchpoint 2 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint2, "Touchpoint 3", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint2_PositionX, "Touchpoint 3 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint2_PositionY, "Touchpoint 3 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint3, "Touchpoint 4", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint3_PositionX, "Touchpoint 4 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint3_PositionY, "Touchpoint 4 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint4, "Touchpoint 5", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint4_PositionX, "Touchpoint 5 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint4_PositionY, "Touchpoint 5 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint5, "Touchpoint 6", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint5_PositionX, "Touchpoint 6 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint5_PositionY, "Touchpoint 6 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint6, "Touchpoint 7", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint6_PositionX, "Touchpoint 7 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint6_PositionY, "Touchpoint 7 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint7, "Touchpoint 8", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint7_PositionX, "Touchpoint 8 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint7_PositionY, "Touchpoint 8 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint8, "Touchpoint 9", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint8_PositionX, "Touchpoint 9 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint8_PositionY, "Touchpoint 9 Position Y", ezInputSlotFlags::IsTouchPosition);

  RegisterInputSlot(ezInputSlot_TouchPoint9, "Touchpoint 10", ezInputSlotFlags::IsTouchPoint);
  RegisterInputSlot(ezInputSlot_TouchPoint9_PositionX, "Touchpoint 10 Position X", ezInputSlotFlags::IsTouchPosition);
  RegisterInputSlot(ezInputSlot_TouchPoint9_PositionY, "Touchpoint 10 Position Y", ezInputSlotFlags::IsTouchPosition);
}

void ezStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[ezInputSlot_MouseWheelUp]  = 0;
  m_InputSlotValues[ezInputSlot_MouseWheelDown]= 0;
  m_InputSlotValues[ezInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosY] = 0;
  m_InputSlotValues[ezInputSlot_MouseDblClick0] = 0;
  m_InputSlotValues[ezInputSlot_MouseDblClick1] = 0;
  m_InputSlotValues[ezInputSlot_MouseDblClick2] = 0;
}

void SetClipRect(bool bClip, HWND hWnd)
{
  // NOT IMPLEMENTED. TODO
}

void ezStandardInputDevice::SetClipMouseCursor(bool bEnable)
{
  if (m_bClipCursor == bEnable)
    return;

  if(bEnable)
    m_coreWindow->SetPointerCapture();
  else
    m_coreWindow->ReleasePointerCapture();

  m_bClipCursor = bEnable;
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  // Hide
  if (!bShow)
  {
    // Save cursor to reinstantiate it.
    m_coreWindow->get_PointerCursor(&m_cursorBeforeHide);
    m_coreWindow->put_PointerCursor(nullptr);
  }

  // Show
  else
  {
    EZ_ASSERT_DEV(m_cursorBeforeHide, "There should be a ICoreCursor backup that can be put back.");
    m_coreWindow->put_PointerCursor(m_cursorBeforeHide.Get());
  }

  m_bShowCursor = bShow;
}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}
