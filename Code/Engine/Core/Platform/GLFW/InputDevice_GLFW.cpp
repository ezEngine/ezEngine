#include <Core/CorePCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_GLFW)

#  include <Core/Platform/GLFW/InputDevice_GLFW.h>
#  include <GLFW/glfw3.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  const char* ConvertGLFWKeyToEngineName(int key)
  {
    switch (key)
    {
      case GLFW_KEY_LEFT:
        return ezInputSlot_KeyLeft;
      case GLFW_KEY_RIGHT:
        return ezInputSlot_KeyRight;
      case GLFW_KEY_UP:
        return ezInputSlot_KeyUp;
      case GLFW_KEY_DOWN:
        return ezInputSlot_KeyDown;
      case GLFW_KEY_ESCAPE:
        return ezInputSlot_KeyEscape;
      case GLFW_KEY_SPACE:
        return ezInputSlot_KeySpace;
      case GLFW_KEY_BACKSPACE:
        return ezInputSlot_KeyBackspace;
      case GLFW_KEY_ENTER:
        return ezInputSlot_KeyReturn;
      case GLFW_KEY_TAB:
        return ezInputSlot_KeyTab;
      case GLFW_KEY_LEFT_SHIFT:
        return ezInputSlot_KeyLeftShift;
      case GLFW_KEY_RIGHT_SHIFT:
        return ezInputSlot_KeyRightShift;
      case GLFW_KEY_LEFT_CONTROL:
        return ezInputSlot_KeyLeftCtrl;
      case GLFW_KEY_RIGHT_CONTROL:
        return ezInputSlot_KeyRightCtrl;
      case GLFW_KEY_LEFT_ALT:
        return ezInputSlot_KeyLeftAlt;
      case GLFW_KEY_RIGHT_ALT:
        return ezInputSlot_KeyRightAlt;
      case GLFW_KEY_LEFT_SUPER:
        return ezInputSlot_KeyLeftWin;
      case GLFW_KEY_RIGHT_SUPER:
        return ezInputSlot_KeyRightWin;
      case GLFW_KEY_MENU:
        return ezInputSlot_KeyApps;
      case GLFW_KEY_LEFT_BRACKET:
        return ezInputSlot_KeyBracketOpen;
      case GLFW_KEY_RIGHT_BRACKET:
        return ezInputSlot_KeyBracketClose;
      case GLFW_KEY_SEMICOLON:
        return ezInputSlot_KeySemicolon;
      case GLFW_KEY_APOSTROPHE:
        return ezInputSlot_KeyApostrophe;
      case GLFW_KEY_SLASH:
        return ezInputSlot_KeySlash;
      case GLFW_KEY_EQUAL:
        return ezInputSlot_KeyEquals;
      case GLFW_KEY_GRAVE_ACCENT:
        return ezInputSlot_KeyTilde;
      case GLFW_KEY_MINUS:
        return ezInputSlot_KeyHyphen;
      case GLFW_KEY_COMMA:
        return ezInputSlot_KeyComma;
      case GLFW_KEY_PERIOD:
        return ezInputSlot_KeyPeriod;
      case GLFW_KEY_BACKSLASH:
        return ezInputSlot_KeyBackslash;
      case GLFW_KEY_WORLD_1:
        return ezInputSlot_KeyPipe;
      case GLFW_KEY_1:
        return ezInputSlot_Key1;
      case GLFW_KEY_2:
        return ezInputSlot_Key2;
      case GLFW_KEY_3:
        return ezInputSlot_Key3;
      case GLFW_KEY_4:
        return ezInputSlot_Key4;
      case GLFW_KEY_5:
        return ezInputSlot_Key5;
      case GLFW_KEY_6:
        return ezInputSlot_Key6;
      case GLFW_KEY_7:
        return ezInputSlot_Key7;
      case GLFW_KEY_8:
        return ezInputSlot_Key8;
      case GLFW_KEY_9:
        return ezInputSlot_Key9;
      case GLFW_KEY_0:
        return ezInputSlot_Key0;
      case GLFW_KEY_KP_1:
        return ezInputSlot_KeyNumpad1;
      case GLFW_KEY_KP_2:
        return ezInputSlot_KeyNumpad2;
      case GLFW_KEY_KP_3:
        return ezInputSlot_KeyNumpad3;
      case GLFW_KEY_KP_4:
        return ezInputSlot_KeyNumpad4;
      case GLFW_KEY_KP_5:
        return ezInputSlot_KeyNumpad5;
      case GLFW_KEY_KP_6:
        return ezInputSlot_KeyNumpad6;
      case GLFW_KEY_KP_7:
        return ezInputSlot_KeyNumpad7;
      case GLFW_KEY_KP_8:
        return ezInputSlot_KeyNumpad8;
      case GLFW_KEY_KP_9:
        return ezInputSlot_KeyNumpad9;
      case GLFW_KEY_KP_0:
        return ezInputSlot_KeyNumpad0;
      case GLFW_KEY_A:
        return ezInputSlot_KeyA;
      case GLFW_KEY_B:
        return ezInputSlot_KeyB;
      case GLFW_KEY_C:
        return ezInputSlot_KeyC;
      case GLFW_KEY_D:
        return ezInputSlot_KeyD;
      case GLFW_KEY_E:
        return ezInputSlot_KeyE;
      case GLFW_KEY_F:
        return ezInputSlot_KeyF;
      case GLFW_KEY_G:
        return ezInputSlot_KeyG;
      case GLFW_KEY_H:
        return ezInputSlot_KeyH;
      case GLFW_KEY_I:
        return ezInputSlot_KeyI;
      case GLFW_KEY_J:
        return ezInputSlot_KeyJ;
      case GLFW_KEY_K:
        return ezInputSlot_KeyK;
      case GLFW_KEY_L:
        return ezInputSlot_KeyL;
      case GLFW_KEY_M:
        return ezInputSlot_KeyM;
      case GLFW_KEY_N:
        return ezInputSlot_KeyN;
      case GLFW_KEY_O:
        return ezInputSlot_KeyO;
      case GLFW_KEY_P:
        return ezInputSlot_KeyP;
      case GLFW_KEY_Q:
        return ezInputSlot_KeyQ;
      case GLFW_KEY_R:
        return ezInputSlot_KeyR;
      case GLFW_KEY_S:
        return ezInputSlot_KeyS;
      case GLFW_KEY_T:
        return ezInputSlot_KeyT;
      case GLFW_KEY_U:
        return ezInputSlot_KeyU;
      case GLFW_KEY_V:
        return ezInputSlot_KeyV;
      case GLFW_KEY_W:
        return ezInputSlot_KeyW;
      case GLFW_KEY_X:
        return ezInputSlot_KeyX;
      case GLFW_KEY_Y:
        return ezInputSlot_KeyY;
      case GLFW_KEY_Z:
        return ezInputSlot_KeyZ;
      case GLFW_KEY_F1:
        return ezInputSlot_KeyF1;
      case GLFW_KEY_F2:
        return ezInputSlot_KeyF2;
      case GLFW_KEY_F3:
        return ezInputSlot_KeyF3;
      case GLFW_KEY_F4:
        return ezInputSlot_KeyF4;
      case GLFW_KEY_F5:
        return ezInputSlot_KeyF5;
      case GLFW_KEY_F6:
        return ezInputSlot_KeyF6;
      case GLFW_KEY_F7:
        return ezInputSlot_KeyF7;
      case GLFW_KEY_F8:
        return ezInputSlot_KeyF8;
      case GLFW_KEY_F9:
        return ezInputSlot_KeyF9;
      case GLFW_KEY_F10:
        return ezInputSlot_KeyF10;
      case GLFW_KEY_F11:
        return ezInputSlot_KeyF11;
      case GLFW_KEY_F12:
        return ezInputSlot_KeyF12;
      case GLFW_KEY_HOME:
        return ezInputSlot_KeyHome;
      case GLFW_KEY_END:
        return ezInputSlot_KeyEnd;
      case GLFW_KEY_DELETE:
        return ezInputSlot_KeyDelete;
      case GLFW_KEY_INSERT:
        return ezInputSlot_KeyInsert;
      case GLFW_KEY_PAGE_UP:
        return ezInputSlot_KeyPageUp;
      case GLFW_KEY_PAGE_DOWN:
        return ezInputSlot_KeyPageDown;
      case GLFW_KEY_NUM_LOCK:
        return ezInputSlot_KeyNumLock;
      case GLFW_KEY_KP_ADD:
        return ezInputSlot_KeyNumpadPlus;
      case GLFW_KEY_KP_SUBTRACT:
        return ezInputSlot_KeyNumpadMinus;
      case GLFW_KEY_KP_MULTIPLY:
        return ezInputSlot_KeyNumpadStar;
      case GLFW_KEY_KP_DIVIDE:
        return ezInputSlot_KeyNumpadSlash;
      case GLFW_KEY_KP_DECIMAL:
        return ezInputSlot_KeyNumpadPeriod;
      case GLFW_KEY_KP_ENTER:
        return ezInputSlot_KeyNumpadEnter;
      case GLFW_KEY_CAPS_LOCK:
        return ezInputSlot_KeyCapsLock;
      case GLFW_KEY_PRINT_SCREEN:
        return ezInputSlot_KeyPrint;
      case GLFW_KEY_SCROLL_LOCK:
        return ezInputSlot_KeyScroll;
      case GLFW_KEY_PAUSE:
        return ezInputSlot_KeyPause;
      // TODO ezInputSlot_KeyPrevTrack
      // TODO ezInputSlot_KeyNextTrack
      // TODO ezInputSlot_KeyPlayPause
      // TODO ezInputSlot_KeyStop
      // TODO ezInputSlot_KeyVolumeUp
      // TODO ezInputSlot_KeyVolumeDown
      // TODO ezInputSlot_KeyMute
      default:
        return nullptr;
    }
  }
} // namespace

ezStandardInputDevice::ezStandardInputDevice(ezUInt32 uiWindowNumber, GLFWwindow* windowHandle)
  : m_uiWindowNumber(uiWindowNumber)
  , m_pWindow(windowHandle)
{
}

ezStandardInputDevice::~ezStandardInputDevice()
{
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  glfwSetInputMode(m_pWindow, GLFW_CURSOR, bShow ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return (glfwGetInputMode(m_pWindow, GLFW_CURSOR) != GLFW_CURSOR_DISABLED);
}

void ezStandardInputDevice::SetClipMouseCursor(ezMouseCursorClipMode::Enum mode)
{
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

void ezStandardInputDevice::OnKey(int key, int scancode, int action, int mods)
{
  if (key == GLFW_KEY_BACKSPACE && (action == GLFW_PRESS || action == GLFW_REPEAT))
  {
    m_uiLastCharacter = 0x00000008;
  }

  // TODO: if (key != scancode) -> use scancode, (ConvertScanCodeToEngineName), only if they are the same, use ConvertGLFWKeyToEngineName

  const char* szInputSlotName = ConvertGLFWKeyToEngineName(key);
  if (szInputSlotName)
  {
    m_InputSlotValues[szInputSlotName] = (action == GLFW_RELEASE) ? 0.0f : 1.0f;
  }
  else
  {
    ezLog::Warning("Unhandeled glfw keyboard key {} {}", key, (action == GLFW_RELEASE) ? "released" : "pressed");
  }
}

void ezStandardInputDevice::OnCharacter(unsigned int codepoint)
{
  m_uiLastCharacter = codepoint;
}

void ezStandardInputDevice::OnCursorPosition(double xpos, double ypos)
{
  s_iMouseIsOverWindowNumber = m_uiWindowNumber;

  int width;
  int height;
  glfwGetWindowSize(m_pWindow, &width, &height);
  const float fInvW = 1.0f / width;
  const float fInvH = 1.0f / height;

  m_InputSlotValues[ezInputSlot_MousePositionX] = static_cast<float>(xpos * fInvW);
  m_InputSlotValues[ezInputSlot_MousePositionY] = static_cast<float>(ypos * fInvH);

  if (m_LastPos.x != ezMath::MaxValue<double>())
  {
    ezVec2d diff = ezVec2d(xpos, ypos) - m_LastPos;

    m_InputSlotValues[ezInputSlot_MouseMoveNegX] += ((diff.x < 0) ? (float)-diff.x : 0.0f) * GetMouseSpeed().x * fInvW;
    m_InputSlotValues[ezInputSlot_MouseMovePosX] += ((diff.x > 0) ? (float)diff.x : 0.0f) * GetMouseSpeed().x * fInvW;
    m_InputSlotValues[ezInputSlot_MouseMoveNegY] += ((diff.y < 0) ? (float)-diff.y : 0.0f) * GetMouseSpeed().y * fInvH;
    m_InputSlotValues[ezInputSlot_MouseMovePosY] += ((diff.y > 0) ? (float)diff.y : 0.0f) * GetMouseSpeed().y * fInvH;
  }
  m_LastPos = ezVec2d(xpos, ypos);
}

void ezStandardInputDevice::OnMouseButton(int button, int action, int mods)
{
  const char* inputSlot = nullptr;
  switch (button)
  {
    case GLFW_MOUSE_BUTTON_1:
      inputSlot = ezInputSlot_MouseButton0;
      break;
    case GLFW_MOUSE_BUTTON_2:
      inputSlot = ezInputSlot_MouseButton1;
      break;
    case GLFW_MOUSE_BUTTON_3:
      inputSlot = ezInputSlot_MouseButton2;
      break;
    case GLFW_MOUSE_BUTTON_4:
      inputSlot = ezInputSlot_MouseButton3;
      break;
    case GLFW_MOUSE_BUTTON_5:
      inputSlot = ezInputSlot_MouseButton4;
      break;
  }

  if (inputSlot)
  {
    m_InputSlotValues[inputSlot] = (action == GLFW_PRESS) ? 1.0f : 0.0f;
  }
}

void ezStandardInputDevice::OnScroll(double xoffset, double yoffset)
{
  if (yoffset > 0)
  {
    m_InputSlotValues[ezInputSlot_MouseWheelUp] = static_cast<float>(yoffset);
  }
  else
  {
    m_InputSlotValues[ezInputSlot_MouseWheelDown] = static_cast<float>(-yoffset);
  }
}

#endif


EZ_STATICLINK_FILE(Core, Core_Platform_GLFW_InputDevice_GLFW);
