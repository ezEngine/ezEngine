#if EZ_ENABLED(EZ_PLATFORM_WEB)

#  include <Core/System/Implementation/Web/InputDevice_Web.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static ezStandardInputDevice* s_pInputDevice = nullptr;
ezArrayMap<ezUInt32, ezStringView> ezStandardInputDevice::s_WebKeyNameToInputSlot;

ezStandardInputDevice::ezStandardInputDevice()
{
  EZ_ASSERT_DEV(s_pInputDevice == nullptr, "Web only allows for one ezStandardInputDevice.");
  s_pInputDevice = this;
}

ezStandardInputDevice::~ezStandardInputDevice()
{
  s_pInputDevice = nullptr;
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (!bShow)
  {
    ezLog::Warning("ezStandardInputDevice::SetShowMouseCursor: not available on this platform.");
  }
}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return true;
}

void ezStandardInputDevice::SetClipMouseCursor(ezMouseCursorClipMode::Enum mode)
{
  ezLog::Warning("ezStandardInputDevice::SetClipMouseCursor: not available on this platform.");
}

ezMouseCursorClipMode::Enum ezStandardInputDevice::GetClipMouseCursor() const
{
  return ezMouseCursorClipMode::Default;
}

void ezStandardInputDevice::InitializeDevice() {}

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
  // TODO RegisterInputSlot(ezInputSlot_KeyTilde, "~", ezInputSlotFlags::IsButton);
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

  /* TODO
  RegisterInputSlot(ezInputSlot_KeyPrevTrack, "Previous Track", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNextTrack, "Next Track", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPlayPause, "Play / Pause", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyStop, "Stop", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyVolumeUp, "Volume Up", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyVolumeDown, "Volume Down", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyMute, "Mute", ezInputSlotFlags::IsButton);
  */

  RegisterInputSlot(ezInputSlot_MousePositionX, "Mouse Position X", ezInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(ezInputSlot_MousePositionY, "Mouse Position Y", ezInputSlotFlags::IsMouseAxisPosition);

  RegisterInputSlot(ezInputSlot_MouseMoveNegX, "Mouse Move Left", ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosX, "Mouse Move Right", ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMoveNegY, "Mouse Move Down", ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosY, "Mouse Move Up", ezInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(ezInputSlot_MouseButton0, "Mousebutton 0", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton1, "Mousebutton 1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton2, "Mousebutton 2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton3, "Mousebutton 3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton4, "Mousebutton 4", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_MouseWheelUp, "Mousewheel Up", ezInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(ezInputSlot_MouseWheelDown, "Mousewheel Down", ezInputSlotFlags::IsMouseWheel);

  auto SetupKey = [&](ezStringView sName, ezStringView sKey)
  {
    const ezUInt32 uiHash = ezHashingUtils::xxHash32String(sName);
    s_WebKeyNameToInputSlot.Insert(uiHash, sKey);
  };

  // see https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values

  SetupKey("KeyA", ezInputSlot_KeyA);
  SetupKey("KeyB", ezInputSlot_KeyB);
  SetupKey("KeyC", ezInputSlot_KeyC);
  SetupKey("KeyD", ezInputSlot_KeyD);
  SetupKey("KeyE", ezInputSlot_KeyE);
  SetupKey("KeyF", ezInputSlot_KeyF);
  SetupKey("KeyG", ezInputSlot_KeyG);
  SetupKey("KeyH", ezInputSlot_KeyH);
  SetupKey("KeyI", ezInputSlot_KeyI);
  SetupKey("KeyJ", ezInputSlot_KeyJ);
  SetupKey("KeyK", ezInputSlot_KeyK);
  SetupKey("KeyL", ezInputSlot_KeyL);
  SetupKey("KeyM", ezInputSlot_KeyM);
  SetupKey("KeyN", ezInputSlot_KeyN);
  SetupKey("KeyO", ezInputSlot_KeyO);
  SetupKey("KeyP", ezInputSlot_KeyP);
  SetupKey("KeyQ", ezInputSlot_KeyQ);
  SetupKey("KeyR", ezInputSlot_KeyR);
  SetupKey("KeyS", ezInputSlot_KeyS);
  SetupKey("KeyT", ezInputSlot_KeyT);
  SetupKey("KeyU", ezInputSlot_KeyU);
  SetupKey("KeyV", ezInputSlot_KeyV);
  SetupKey("KeyW", ezInputSlot_KeyW);
  SetupKey("KeyX", ezInputSlot_KeyX);
  SetupKey("KeyY", ezInputSlot_KeyY);
  SetupKey("KeyZ", ezInputSlot_KeyZ);

  SetupKey("Quote", ezInputSlot_KeyApostrophe);
  SetupKey("Backslash", ezInputSlot_KeyBackslash);
  SetupKey("Backspace", ezInputSlot_KeyBackspace);
  SetupKey("CapsLock", ezInputSlot_KeyCapsLock);
  SetupKey("Comma", ezInputSlot_KeyComma);
  SetupKey("Delete", ezInputSlot_KeyDelete);
  SetupKey("ArrowDown", ezInputSlot_KeyDown);

  SetupKey("End", ezInputSlot_KeyEnd);
  SetupKey("Enter", ezInputSlot_KeyReturn);

  SetupKey("Equal", ezInputSlot_KeyEquals);

  SetupKey("Escape", ezInputSlot_KeyEscape);

  SetupKey("F1", ezInputSlot_KeyF1);
  SetupKey("F2", ezInputSlot_KeyF2);
  SetupKey("F3", ezInputSlot_KeyF3);
  SetupKey("F4", ezInputSlot_KeyF4);
  SetupKey("F5", ezInputSlot_KeyF5);
  SetupKey("F6", ezInputSlot_KeyF6);
  SetupKey("F7", ezInputSlot_KeyF7);
  SetupKey("F8", ezInputSlot_KeyF8);
  SetupKey("F9", ezInputSlot_KeyF9);
  SetupKey("F10", ezInputSlot_KeyF10);
  SetupKey("F11", ezInputSlot_KeyF11);
  SetupKey("F12", ezInputSlot_KeyF12);

  SetupKey("Backquote", ezInputSlot_KeyTilde);
  SetupKey("Home", ezInputSlot_KeyHome);
  SetupKey("Insert", ezInputSlot_KeyInsert);

  SetupKey("Numpad0", ezInputSlot_KeyNumpad0);
  SetupKey("Numpad1", ezInputSlot_KeyNumpad1);
  SetupKey("Numpad2", ezInputSlot_KeyNumpad2);
  SetupKey("Numpad3", ezInputSlot_KeyNumpad3);
  SetupKey("Numpad4", ezInputSlot_KeyNumpad4);
  SetupKey("Numpad5", ezInputSlot_KeyNumpad5);
  SetupKey("Numpad6", ezInputSlot_KeyNumpad6);
  SetupKey("Numpad7", ezInputSlot_KeyNumpad7);
  SetupKey("Numpad8", ezInputSlot_KeyNumpad8);
  SetupKey("Numpad9", ezInputSlot_KeyNumpad9);

  SetupKey("NumpadAdd", ezInputSlot_KeyNumpadPlus);
  SetupKey("NumpadDecimal", ezInputSlot_KeyNumpadPeriod);
  SetupKey("NumpadComma", ezInputSlot_KeyNumpadPeriod); // intentionally duplicate mapping
  SetupKey("NumpadDivide", ezInputSlot_KeyNumpadSlash);
  SetupKey("NumpadEnter", ezInputSlot_KeyNumpadEnter);
  SetupKey("NumpadMultiply", ezInputSlot_KeyNumpadStar);
  SetupKey("NumpadSubtract", ezInputSlot_KeyNumpadMinus);

  SetupKey("ArrowLeft", ezInputSlot_KeyLeft);
  SetupKey("AltLeft", ezInputSlot_KeyLeftAlt);
  SetupKey("BracketLeft", ezInputSlot_KeyBracketOpen);
  SetupKey("ControlLeft", ezInputSlot_KeyLeftCtrl);
  SetupKey("ShiftLeft", ezInputSlot_KeyLeftShift);
  SetupKey("MetaLeft", ezInputSlot_KeyLeftWin);

  SetupKey("ContextMenu", ezInputSlot_KeyApps);
  // SetupKey("Minus", ezInputSlot_KeyMinus);

  SetupKey("Digit0", ezInputSlot_Key0);
  SetupKey("Digit1", ezInputSlot_Key1);
  SetupKey("Digit2", ezInputSlot_Key2);
  SetupKey("Digit3", ezInputSlot_Key3);
  SetupKey("Digit4", ezInputSlot_Key4);
  SetupKey("Digit5", ezInputSlot_Key5);
  SetupKey("Digit6", ezInputSlot_Key6);
  SetupKey("Digit7", ezInputSlot_Key7);
  SetupKey("Digit8", ezInputSlot_Key8);
  SetupKey("Digit9", ezInputSlot_Key9);

  SetupKey("NumLock", ezInputSlot_KeyNumLock);
  SetupKey("PageDown", ezInputSlot_KeyPageDown);
  SetupKey("PageUp", ezInputSlot_KeyPageUp);
  SetupKey("Pause", ezInputSlot_KeyPause);
  SetupKey("Period", ezInputSlot_KeyPeriod);
  SetupKey("PrintScreen", ezInputSlot_KeyPrint);

  SetupKey("ArrowRight", ezInputSlot_KeyRight);
  SetupKey("AltRight", ezInputSlot_KeyRightAlt);
  SetupKey("BracketRight", ezInputSlot_KeyBracketClose);
  SetupKey("ControlRight", ezInputSlot_KeyRightCtrl);
  SetupKey("ShiftRight", ezInputSlot_KeyRightShift);
  SetupKey("MetaRight", ezInputSlot_KeyRightWin);

  SetupKey("ScrollLock", ezInputSlot_KeyScroll);
  SetupKey("Semicolon", ezInputSlot_KeySemicolon);
  SetupKey("Slash", ezInputSlot_KeySlash);
  SetupKey("Space", ezInputSlot_KeySpace);

  SetupKey("Tab", ezInputSlot_KeyTab);
  SetupKey("ArrowUp", ezInputSlot_KeyUp);
  SetupKey("IntlBackslash", ezInputSlot_KeyPipe);

  s_WebKeyNameToInputSlot.Sort();
}

void ezStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[ezInputSlot_MouseWheelUp] = 0;
  m_InputSlotValues[ezInputSlot_MouseWheelDown] = 0;
  m_InputSlotValues[ezInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosY] = 0;
}

void ezStandardInputDevice::onWebChar(const std::string& text)
{
  if (s_pInputDevice == nullptr)
    return;

  ezStringBuilder tmp = text.c_str();
  if (tmp.IsEmpty())
    return;

  s_pInputDevice->m_uiLastCharacter = tmp.GetView().GetIteratorFront().GetCharacter();

  // ezLog::Info("Char '{}'", text.c_str());
}

void ezStandardInputDevice::onWebKey(const std::string& name, bool bDown)
{
  if (s_pInputDevice == nullptr)
    return;

  const ezUInt32 hash = ezHashingUtils::xxHash32String(name.c_str());
  const ezUInt32 idx = s_WebKeyNameToInputSlot.Find(hash);

  if (idx == ezInvalidIndex)
    return;

  ezStringView sInputSlotName = s_WebKeyNameToInputSlot.GetValue(idx);
  s_pInputDevice->m_InputSlotValues[sInputSlotName] = bDown ? 1.0f : 0.0f;

  ezLog::Info("Key '{}' - {}", sInputSlotName, bDown);
}

void ezStandardInputDevice::onWebMouseClick(ezInt32 iButton, bool bDown)
{
  if (s_pInputDevice == nullptr)
    return;

  const char* inputSlot = nullptr;
  switch (iButton)
  {
    case 0:
      inputSlot = ezInputSlot_MouseButton0;
      break;
    case 1:
      inputSlot = ezInputSlot_MouseButton2; // button '2' is the right button and button '1' is middle
      break;
    case 2:
      inputSlot = ezInputSlot_MouseButton1;
      break;
    case 3:
      inputSlot = ezInputSlot_MouseButton3;
      break;
    case 4:
      inputSlot = ezInputSlot_MouseButton4;
      break;
  }

  if (inputSlot)
  {
    s_pInputDevice->m_InputSlotValues[inputSlot] = bDown ? 1.0f : 0.0f;

    ezLog::Info("Click '{}' - {}", inputSlot, bDown);
  }
}

void ezStandardInputDevice::onWebMouseMove(double x, double y)
{
  if (s_pInputDevice == nullptr)
    return;

  s_pInputDevice->m_InputSlotValues[ezInputSlot_MousePositionX] = static_cast<float>(x);
  s_pInputDevice->m_InputSlotValues[ezInputSlot_MousePositionY] = static_cast<float>(y);

  if (s_pInputDevice->m_LastPos.x != ezMath::MaxValue<double>())
  {
    const ezVec2d diff = ezVec2d(x, y) - s_pInputDevice->m_LastPos;

    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseMoveNegX] += ((diff.x < 0) ? (float)-diff.x : 0.0f) * s_pInputDevice->GetMouseSpeed().x;
    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseMovePosX] += ((diff.x > 0) ? (float)diff.x : 0.0f) * s_pInputDevice->GetMouseSpeed().x;
    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseMoveNegY] += ((diff.y < 0) ? (float)-diff.y : 0.0f) * s_pInputDevice->GetMouseSpeed().y;
    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseMovePosY] += ((diff.y > 0) ? (float)diff.y : 0.0f) * s_pInputDevice->GetMouseSpeed().y;

    // ezLog::Info("Mouse Move: {} / {} - {} / {}", x, y, diff.x, diff.y);
  }

  s_pInputDevice->m_LastPos = ezVec2d(x, y);
}

void ezStandardInputDevice::onWebMouseLeave()
{
  if (s_pInputDevice == nullptr)
    return;

  // reset mouse button states
  s_pInputDevice->onWebMouseClick(0, false);
  s_pInputDevice->onWebMouseClick(1, false);
  s_pInputDevice->onWebMouseClick(2, false);
  s_pInputDevice->onWebMouseClick(3, false);
  s_pInputDevice->onWebMouseClick(4, false);

  ezLog::Info("Mouse leave");
}

void ezStandardInputDevice::onWebMouseWheel(double y)
{
  if (s_pInputDevice == nullptr)
    return;

  if (y > 0)
  {
    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseWheelUp] = static_cast<float>(y);
  }
  else
  {
    s_pInputDevice->m_InputSlotValues[ezInputSlot_MouseWheelDown] = static_cast<float>(-y);
  }

  ezLog::Info("Mouse wheel: {}", y);
}

#  include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(input)
{
  function("onWebChar", &ezStandardInputDevice::onWebChar);
  function("onWebKey", &ezStandardInputDevice::onWebKey);
  function("onWebMouseClick", &ezStandardInputDevice::onWebMouseClick);
  function("onWebMouseMove", &ezStandardInputDevice::onWebMouseMove);
  function("onWebMouseLeave", &ezStandardInputDevice::onWebMouseLeave);
  function("onWebMouseWheel", &ezStandardInputDevice::onWebMouseWheel);
}

#endif


EZ_STATICLINK_FILE(Core, Core_System_Implementation_Web_InputDevice_Web);
