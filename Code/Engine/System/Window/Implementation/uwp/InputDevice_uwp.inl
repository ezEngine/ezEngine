#include <Core/PCH.h>
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
    m_coreWindow->remove_KeyDown(m_keyDownEventRegistration);
    m_coreWindow->remove_KeyDown(m_keyUpEventRegistration);
  }
}

void ezStandardInputDevice::InitializeDevice()
{
  using KeyHandler = __FITypedEventHandler_2_Windows__CUI__CCore__CCoreWindow_Windows__CUI__CCore__CKeyEventArgs;

  m_coreWindow->add_KeyDown(Callback<KeyHandler>(this, &ezStandardInputDevice::OnKeyEvent).Get(), &m_keyDownEventRegistration);
  m_coreWindow->add_KeyUp(Callback<KeyHandler>(this, &ezStandardInputDevice::OnKeyEvent).Get(), &m_keyUpEventRegistration);
}

HRESULT ezStandardInputDevice::OnKeyEvent(ICoreWindow* coreWindow, IKeyEventArgs* args)
{
  // Closely related to the RawInput implementation in Win32/InputDevice_win32.inl

  CorePhysicalKeyStatus keyStatus;
  EZ_SUCCEED_OR_RETURN_HRESULT(args->get_KeyStatus(&keyStatus));

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
  //RegisterInputSlot(ezInputSlot_TouchPoint0, "Touchpoint 1", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint0_PositionX, "Touchpoint 1 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint0_PositionY, "Touchpoint 1 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint1, "Touchpoint 2", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint1_PositionX, "Touchpoint 2 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint1_PositionY, "Touchpoint 2 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint2, "Touchpoint 3", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint2_PositionX, "Touchpoint 3 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint2_PositionY, "Touchpoint 3 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint3, "Touchpoint 4", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint3_PositionX, "Touchpoint 4 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint3_PositionY, "Touchpoint 4 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint4, "Touchpoint 5", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint4_PositionX, "Touchpoint 5 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint4_PositionY, "Touchpoint 5 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint5, "Touchpoint 6", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint5_PositionX, "Touchpoint 6 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint5_PositionY, "Touchpoint 6 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint6, "Touchpoint 7", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint6_PositionX, "Touchpoint 7 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint6_PositionY, "Touchpoint 7 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint7, "Touchpoint 8", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint7_PositionX, "Touchpoint 8 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint7_PositionY, "Touchpoint 8 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint8, "Touchpoint 9", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint8_PositionX, "Touchpoint 9 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint8_PositionY, "Touchpoint 9 Position Y", ezInputSlotFlags::IsTouchPosition);

  //RegisterInputSlot(ezInputSlot_TouchPoint9, "Touchpoint 10", ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint9_PositionX, "Touchpoint 10 Position X", ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint9_PositionY, "Touchpoint 10 Position Y", ezInputSlotFlags::IsTouchPosition);
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
  m_bClipCursor = bEnable;

  // NOT IMPLEMENTED. TODO
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;

  // NOT IMPLEMENTED. TODO
}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

#include <System/Window/Implementation/Win32/WindowsScanCodes_inl.h>
