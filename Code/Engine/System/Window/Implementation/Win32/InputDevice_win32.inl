#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <System/Window/Implementation/Win32/InputDevice_win32.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Containers/HybridArray.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, ezInputDeviceMouseKeyboard, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

bool ezStandardInputDevice::s_bMainWindowUsed = false;

ezStandardInputDevice::ezStandardInputDevice(ezUInt32 uiWindowNumber)
{
  m_uiWindowNumber = uiWindowNumber;
  m_bClipCursor = false;
  m_bShowCursor = true;

  if (uiWindowNumber == 0)
  {
    EZ_ASSERT_RELEASE(!s_bMainWindowUsed, "You cannot have two devices of Type ezStandardInputDevice with the window number zero.");
    ezStandardInputDevice::s_bMainWindowUsed = true;
  }
}

ezStandardInputDevice::~ezStandardInputDevice()
{
  if (m_uiWindowNumber == 0)
    ezStandardInputDevice::s_bMainWindowUsed = false;
}

void ezStandardInputDevice::InitializeDevice()
{
  if (m_uiWindowNumber == 0)
  {
    RAWINPUTDEVICE Rid[2];

    // keyboard
    Rid[0].usUsagePage = 0x01; 
    Rid[0].usUsage = 0x06; 
    Rid[0].dwFlags = RIDEV_NOHOTKEYS; // Disables Windows-Key and Application-Key
    Rid[0].hwndTarget = nullptr;

    // mouse
    Rid[1].usUsagePage = 0x01; 
    Rid[1].usUsage = 0x02; 
    Rid[1].dwFlags = 0;
    Rid[1].hwndTarget = nullptr;

    if (RegisterRawInputDevices(&Rid[0], (UINT) 2, sizeof(RAWINPUTDEVICE)) == FALSE) 
    {
      ezLog::Error("Could not initialize RawInput for Mouse and Keyboard input.");
    }
    else
      ezLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
  else
    ezLog::Info("Window %i does not need to initialize Mouse or Keyboard.", m_uiWindowNumber);
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
  
  RegisterInputSlot(ezInputSlot_KeyPrevTrack, "Previous Track", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNextTrack, "Next Track", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPlayPause, "Play / Pause", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyStop, "Stop", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyVolumeUp, "Volume Up", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyVolumeDown, "Volume Down", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyMute, "Mute", ezInputSlotFlags::IsButton);

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
  if (bClip)
  {
    RECT area;
    GetClientRect(hWnd, &area);

    POINT p0, p1;
    p0.x = 0; p0.y = 0;
    p1.x = area.right; p1.y = area.bottom;

    ClientToScreen(hWnd, &p0);
    ClientToScreen(hWnd, &p1);

    RECT r;
    r.top = p0.y;
    r.left = p0.x;
    r.right = p1.x;
    r.bottom = p1.y;

    ClipCursor(&r);
  }
  else
    ClipCursor(nullptr);
}

void ezStandardInputDevice::SetClipMouseCursor(bool bEnable)
{
  m_bClipCursor = bEnable;

  if (!bEnable)
    ClipCursor(nullptr);
}

// WM_INPUT mouse clicks do not work in some VMs.
// When this is enabled, mouse clicks are retrieved via standard WM_LBUTTONDOWN.
#define EZ_MOUSEBUTTON_COMPATIBILTY_MODE EZ_ON

void ezStandardInputDevice::WindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
#if EZ_ENABLED(EZ_MOUSEBUTTON_COMPATIBILTY_MODE)
  static ezInt32 s_iMouseCaptureCount = 0;
#endif

  switch (Msg)
  {
  case WM_MOUSEWHEEL:
    {
      // The mousewheel does not work with rawinput over touchpads (at least not all)
      // So we handle that one individually

      const ezInt32 iRotated = (ezInt16) HIWORD(wParam);

      if (iRotated > 0)
        m_InputSlotValues[ezInputSlot_MouseWheelUp]   =  iRotated /  120.0f;
      else
        m_InputSlotValues[ezInputSlot_MouseWheelDown] =  iRotated / -120.0f;
    }
    break;
  case WM_MOUSEMOVE:
    {
      RECT area;
      GetClientRect(hWnd, &area);

      const ezUInt32 uiResX = area.right - area.left;
      const ezUInt32 uiResY = area.bottom - area.top;

      const float fPosX = (float)((short)LOWORD(lParam));
      const float fPosY = (float)((short)HIWORD(lParam));

      m_InputSlotValues[ezInputSlot_MousePositionX] = (fPosX / uiResX) + m_uiWindowNumber;
      m_InputSlotValues[ezInputSlot_MousePositionY] = (fPosY / uiResY);
    }
    break;

  case WM_SETFOCUS:
    {
      SetClipRect(m_bClipCursor, hWnd);

    }
    break;
  case WM_KILLFOCUS:
    {
      SetClipRect(false, hWnd);

      auto it = m_InputSlotValues.GetIterator();

      while (it.IsValid())
      {
        it.Value() = 0.0f;
        it.Next();
      }
    }
    return;
  case WM_CHAR:
    m_LastCharacter = (wchar_t) wParam;
    return;
  case WM_LBUTTONDBLCLK:
    m_InputSlotValues[ezInputSlot_MouseDblClick0] = 1.0f;
    return;
  case WM_RBUTTONDBLCLK:
    m_InputSlotValues[ezInputSlot_MouseDblClick1] = 1.0f;
    return;
  case WM_MBUTTONDBLCLK:
    m_InputSlotValues[ezInputSlot_MouseDblClick2] = 1.0f;
    return;

#if EZ_ENABLED(EZ_MOUSEBUTTON_COMPATIBILTY_MODE)

  case WM_LBUTTONDOWN:
    m_InputSlotValues["mouse_button_0"] = 1.0f;

    if (s_iMouseCaptureCount == 0)
      SetCapture(hWnd);
    ++s_iMouseCaptureCount;

    return;

  case WM_LBUTTONUP:
    m_InputSlotValues["mouse_button_0"] = 0.0f;
    SetClipRect(m_bClipCursor, hWnd);

    --s_iMouseCaptureCount;
    if (s_iMouseCaptureCount <= 0)
      ReleaseCapture();

    return;

  case WM_RBUTTONDOWN:
    m_InputSlotValues["mouse_button_1"] = 1.0f;

    if (s_iMouseCaptureCount == 0)
      SetCapture(hWnd);
    ++s_iMouseCaptureCount;

    return;

  case WM_RBUTTONUP:
    m_InputSlotValues["mouse_button_1"] = 0.0f;
    SetClipRect(m_bClipCursor, hWnd);

    --s_iMouseCaptureCount;
    if (s_iMouseCaptureCount <= 0)
      ReleaseCapture();

    return;

  case WM_MBUTTONDOWN:
    m_InputSlotValues["mouse_button_2"] = 1.0f;

    if (s_iMouseCaptureCount == 0)
      SetCapture(hWnd);
    ++s_iMouseCaptureCount;
    return;

  case WM_MBUTTONUP:
    m_InputSlotValues["mouse_button_2"] = 0.0f;

    --s_iMouseCaptureCount;
    if (s_iMouseCaptureCount <= 0)
      ReleaseCapture();

    return;

  case WM_XBUTTONDOWN:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
      m_InputSlotValues["mouse_button_3"] = 1.0f;
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
      m_InputSlotValues["mouse_button_4"] = 1.0f;

    if (s_iMouseCaptureCount == 0)
      SetCapture(hWnd);
    ++s_iMouseCaptureCount;

    return;

  case WM_XBUTTONUP:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
      m_InputSlotValues["mouse_button_3"] = 0.0f;
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2)
      m_InputSlotValues["mouse_button_4"] = 0.0f;

    --s_iMouseCaptureCount;
    if (s_iMouseCaptureCount <= 0)
      ReleaseCapture();

    return;

  case WM_CAPTURECHANGED: // Sent to the window that is losing the mouse capture.
    s_iMouseCaptureCount = 0;
    return;

#else

  case WM_LBUTTONUP:
    SetClipRect(m_bClipCursor, hWnd);
    return;

#endif

  case WM_INPUT:
    {
      ezUInt32 uiSize = 0;

      GetRawInputData((HRAWINPUT) lParam, RID_INPUT, nullptr, &uiSize, sizeof(RAWINPUTHEADER));

      if (uiSize == 0)
        return;

      ezHybridArray<ezUInt8, sizeof(RAWINPUT)> InputData;
      InputData.SetCount(uiSize);

      if (GetRawInputData((HRAWINPUT) lParam, RID_INPUT, &InputData[0], &uiSize, sizeof(RAWINPUTHEADER)) != uiSize)
        return;

      RAWINPUT* raw = (RAWINPUT*) &InputData[0];

      if (raw->header.dwType == RIM_TYPEKEYBOARD) 
      {
        static bool bIgnoreNext = false;

        if (bIgnoreNext)
        {
          bIgnoreNext = false;
          return;
        }

        static bool bWasStupidLeftShift = false;

        const ezUInt32 uiScanCode = raw->data.keyboard.MakeCode;
        const bool bIsExtended = (raw->data.keyboard.Flags & RI_KEY_E0) != 0;

        if (uiScanCode == 42 && bIsExtended) // 42 has to be special I guess
        {
          //ezLog::Info("Ignored Key: Value: %i, Flags: 0x%X", raw->data.keyboard.MakeCode, raw->data.keyboard.Flags);
          bWasStupidLeftShift = true;
          return;
        }

        const char* szInputSlotName = ezInputManager::ConvertScanCodeToEngineName(uiScanCode, bIsExtended);

        // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
        // so we need to fix this manually
        if (raw->data.keyboard.Flags & RI_KEY_E1)
        {
          szInputSlotName = ezInputSlot_KeyPause;
          bIgnoreNext = true;
        }

        // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
        // we ignore the first stupid shift key entirely and then modify the following Numpad* key
        // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
        // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
        if (ezStringUtils::IsEqual(szInputSlotName, ezInputSlot_KeyNumpadStar) && bWasStupidLeftShift)
          szInputSlotName = ezInputSlot_KeyPrint;

        bWasStupidLeftShift = false;

        int iRequest = raw->data.keyboard.MakeCode << 16;

        if (raw->data.keyboard.Flags & RI_KEY_E0) 
          iRequest |= 1 << 24;

        const bool bPressed = !(raw->data.keyboard.Flags & 0x01);

        m_InputSlotValues[szInputSlotName] = bPressed ? 1.0f : 0.0f;

        if ((m_InputSlotValues[ezInputSlot_KeyLeftCtrl] > 0.1f) && (m_InputSlotValues[ezInputSlot_KeyLeftAlt] > 0.1f) && (m_InputSlotValues[ezInputSlot_KeyNumpadEnter] > 0.1f))
          SetClipMouseCursor(!m_bClipCursor);
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        const ezUInt32 uiButtons = raw->data.mouse.usButtonFlags;

        // "absolute" positions are only reported by devices such as Pens
        // if at all, we should handle them as touch points, not as mouse positions
        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
        {
          m_InputSlotValues[ezInputSlot_MouseMoveNegX] += ((raw->data.mouse.lLastX < 0) ? (float) -raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[ezInputSlot_MouseMovePosX] += ((raw->data.mouse.lLastX > 0) ? (float)  raw->data.mouse.lLastX : 0.0f) * GetMouseSpeed().x;
          m_InputSlotValues[ezInputSlot_MouseMoveNegY] += ((raw->data.mouse.lLastY < 0) ? (float) -raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;
          m_InputSlotValues[ezInputSlot_MouseMovePosY] += ((raw->data.mouse.lLastY > 0) ? (float)  raw->data.mouse.lLastY : 0.0f) * GetMouseSpeed().y;

          // Mouse input does not always work via WM_INPUT
          // e.g. some VMs don't send mouse click input via WM_INPUT when the mouse cursor is visible
          // therefore in 'compatibility mode' it is just queried via standard WM_LBUTTONDOWN etc.
          // to get 'high performance' mouse clicks, this code would work fine though
          // but I doubt it makes much difference in latency
          #if EZ_DISABLED(EZ_MOUSEBUTTON_COMPATIBILTY_MODE)
            for (ezInt32 mb = 0; mb < 5; ++mb)
            {
              char szTemp[32];
              ezStringUtils::snprintf(szTemp, 32, "mouse_button_%i", mb);

              if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2))) != 0)
                m_InputSlotValues[szTemp] = 1.0f;
          
              if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2 + 1))) != 0)
                m_InputSlotValues[szTemp] = 0.0f;
            }
          #endif
        }
        else
        if ((raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
        {
          static int iTouchPoint = 0;
          static bool bTouchPointDown = false;

          const char* szSlot  = ezInputSlot_TouchPoint0;
          const char* szSlotX = ezInputSlot_TouchPoint0_PositionX;
          const char* szSlotY = ezInputSlot_TouchPoint0_PositionY;

          switch (iTouchPoint)
          {
          case 1:
            szSlot  = ezInputSlot_TouchPoint1;
            szSlotX = ezInputSlot_TouchPoint1_PositionX;
            szSlotY = ezInputSlot_TouchPoint1_PositionY;
            break;
          case 2:
            szSlot  = ezInputSlot_TouchPoint2;
            szSlotX = ezInputSlot_TouchPoint2_PositionX;
            szSlotY = ezInputSlot_TouchPoint2_PositionY;
            break;
          case 3:
            szSlot  = ezInputSlot_TouchPoint3;
            szSlotX = ezInputSlot_TouchPoint3_PositionX;
            szSlotY = ezInputSlot_TouchPoint3_PositionY;
            break;
          case 4:
            szSlot  = ezInputSlot_TouchPoint4;
            szSlotX = ezInputSlot_TouchPoint4_PositionX;
            szSlotY = ezInputSlot_TouchPoint4_PositionY;
            break;
          }

          m_InputSlotValues[szSlotX] = (raw->data.mouse.lLastX / 65535.0f) + m_uiWindowNumber;
          m_InputSlotValues[szSlotY] = (raw->data.mouse.lLastY / 65535.0f);

          if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN | RI_MOUSE_BUTTON_2_DOWN)) != 0)
          {
            bTouchPointDown = true;
            m_InputSlotValues[szSlot] = 1.0f;
          }

          if ((uiButtons & (RI_MOUSE_BUTTON_1_UP | RI_MOUSE_BUTTON_2_UP)) != 0)
          {
            bTouchPointDown = false;
            m_InputSlotValues[szSlot] = 0.0f;
          }
        }
        else
        {
          ezLog::Info("Unknown Mouse Move: %.1f | %.1f, Flags = %i", (float) raw->data.mouse.lLastX, (float) raw->data.mouse.lLastY, raw->data.mouse.usFlags);
        }
      } 
      
    }
  }
}


static void SetKeyNameForScanCode(int iScanCode, bool bExtended, const char* szInputSlot)
{
  const ezUInt32 uiKeyCode = (iScanCode << 16) | (bExtended ? (1 << 24) : 0);
  
  wchar_t szKeyName[32] = { 0 };
  GetKeyNameTextW(uiKeyCode, szKeyName, 30);

  ezStringUtf8 sName = szKeyName;

  ezLog::Dev("Translated '%s' to '%s'", ezInputManager::GetInputSlotDisplayName(szInputSlot), sName.GetData());

  ezInputManager::SetInputSlotDisplayName(szInputSlot, sName.GetData());
}

void ezStandardInputDevice::LocalizeButtonDisplayNames()
{
  EZ_LOG_BLOCK("ezStandardInputDevice::LocalizeButtonDisplayNames");

  SetKeyNameForScanCode( 1, false, ezInputSlot_KeyEscape);
  SetKeyNameForScanCode( 2, false, ezInputSlot_Key1);
  SetKeyNameForScanCode( 3, false, ezInputSlot_Key2);
  SetKeyNameForScanCode( 4, false, ezInputSlot_Key3);
  SetKeyNameForScanCode( 5, false, ezInputSlot_Key4);
  SetKeyNameForScanCode( 6, false, ezInputSlot_Key5);
  SetKeyNameForScanCode( 7, false, ezInputSlot_Key6);
  SetKeyNameForScanCode( 8, false, ezInputSlot_Key7);
  SetKeyNameForScanCode( 9, false, ezInputSlot_Key8);
  SetKeyNameForScanCode(10, false, ezInputSlot_Key9);
  SetKeyNameForScanCode(11, false, ezInputSlot_Key0);

  SetKeyNameForScanCode(12, false, ezInputSlot_KeyHyphen);
  SetKeyNameForScanCode(13, false, ezInputSlot_KeyEquals);
  SetKeyNameForScanCode(14, false, ezInputSlot_KeyBackspace);

  SetKeyNameForScanCode(15, false, ezInputSlot_KeyTab);
  SetKeyNameForScanCode(16, false, ezInputSlot_KeyQ);
  SetKeyNameForScanCode(17, false, ezInputSlot_KeyW);
  SetKeyNameForScanCode(18, false, ezInputSlot_KeyE);
  SetKeyNameForScanCode(19, false, ezInputSlot_KeyR);
  SetKeyNameForScanCode(20, false, ezInputSlot_KeyT);
  SetKeyNameForScanCode(21, false, ezInputSlot_KeyY);
  SetKeyNameForScanCode(22, false, ezInputSlot_KeyU);
  SetKeyNameForScanCode(23, false, ezInputSlot_KeyI);
  SetKeyNameForScanCode(24, false, ezInputSlot_KeyO);
  SetKeyNameForScanCode(25, false, ezInputSlot_KeyP);
  SetKeyNameForScanCode(26, false, ezInputSlot_KeyBracketOpen);
  SetKeyNameForScanCode(27, false, ezInputSlot_KeyBracketClose);
  SetKeyNameForScanCode(28, false, ezInputSlot_KeyReturn);

  SetKeyNameForScanCode(29, false, ezInputSlot_KeyLeftCtrl);
  SetKeyNameForScanCode(30, false, ezInputSlot_KeyA);
  SetKeyNameForScanCode(31, false, ezInputSlot_KeyS);
  SetKeyNameForScanCode(32, false, ezInputSlot_KeyD);
  SetKeyNameForScanCode(33, false, ezInputSlot_KeyF);
  SetKeyNameForScanCode(34, false, ezInputSlot_KeyG);
  SetKeyNameForScanCode(35, false, ezInputSlot_KeyH);
  SetKeyNameForScanCode(36, false, ezInputSlot_KeyJ);
  SetKeyNameForScanCode(37, false, ezInputSlot_KeyK);
  SetKeyNameForScanCode(38, false, ezInputSlot_KeyL);
  SetKeyNameForScanCode(39, false, ezInputSlot_KeySemicolon);
  SetKeyNameForScanCode(40, false, ezInputSlot_KeyApostrophe);

  SetKeyNameForScanCode(41, false, ezInputSlot_KeyTilde);
  SetKeyNameForScanCode(42, false, ezInputSlot_KeyLeftShift);
  SetKeyNameForScanCode(43, false, ezInputSlot_KeyBackslash);

  SetKeyNameForScanCode(44, false, ezInputSlot_KeyZ);
  SetKeyNameForScanCode(45, false, ezInputSlot_KeyX);
  SetKeyNameForScanCode(46, false, ezInputSlot_KeyC);
  SetKeyNameForScanCode(47, false, ezInputSlot_KeyV);
  SetKeyNameForScanCode(48, false, ezInputSlot_KeyB);
  SetKeyNameForScanCode(49, false, ezInputSlot_KeyN);
  SetKeyNameForScanCode(50, false, ezInputSlot_KeyM);
  SetKeyNameForScanCode(51, false, ezInputSlot_KeyComma);
  SetKeyNameForScanCode(52, false, ezInputSlot_KeyPeriod);
  SetKeyNameForScanCode(53, false, ezInputSlot_KeySlash);
  SetKeyNameForScanCode(54, false, ezInputSlot_KeyRightShift);
  
  SetKeyNameForScanCode(55, false, ezInputSlot_KeyNumpadStar); // Overlaps with Print

  SetKeyNameForScanCode(56, false, ezInputSlot_KeyLeftAlt);
  SetKeyNameForScanCode(57, false, ezInputSlot_KeySpace);
  SetKeyNameForScanCode(58, false, ezInputSlot_KeyCapsLock);

  SetKeyNameForScanCode(59, false, ezInputSlot_KeyF1);
  SetKeyNameForScanCode(60, false, ezInputSlot_KeyF2);
  SetKeyNameForScanCode(61, false, ezInputSlot_KeyF3);
  SetKeyNameForScanCode(62, false, ezInputSlot_KeyF4);
  SetKeyNameForScanCode(63, false, ezInputSlot_KeyF5);
  SetKeyNameForScanCode(64, false, ezInputSlot_KeyF6);
  SetKeyNameForScanCode(65, false, ezInputSlot_KeyF7);
  SetKeyNameForScanCode(66, false, ezInputSlot_KeyF8);
  SetKeyNameForScanCode(67, false, ezInputSlot_KeyF9);
  SetKeyNameForScanCode(68, false, ezInputSlot_KeyF10);

  SetKeyNameForScanCode(69, true, ezInputSlot_KeyNumLock);  // Prints 'Pause' if it is not 'extended'
  SetKeyNameForScanCode(70, false, ezInputSlot_KeyScroll);  // This overlaps with Pause

  SetKeyNameForScanCode(71, false, ezInputSlot_KeyNumpad7); // This overlaps with Home
  SetKeyNameForScanCode(72, false, ezInputSlot_KeyNumpad8); // This overlaps with Arrow Up
  SetKeyNameForScanCode(73, false, ezInputSlot_KeyNumpad9); // This overlaps with Page Up
  SetKeyNameForScanCode(74, false, ezInputSlot_KeyNumpadMinus);

  SetKeyNameForScanCode(75, false, ezInputSlot_KeyNumpad4); // This overlaps with Arrow Left
  SetKeyNameForScanCode(76, false, ezInputSlot_KeyNumpad5);
  SetKeyNameForScanCode(77, false, ezInputSlot_KeyNumpad6); // This overlaps with Arrow Right
  SetKeyNameForScanCode(78, false, ezInputSlot_KeyNumpadPlus);

  SetKeyNameForScanCode(79, false, ezInputSlot_KeyNumpad1); // This overlaps with End
  SetKeyNameForScanCode(80, false, ezInputSlot_KeyNumpad2); // This overlaps with Arrow Down
  SetKeyNameForScanCode(81, false, ezInputSlot_KeyNumpad3); // This overlaps with Page Down
  SetKeyNameForScanCode(82, false, ezInputSlot_KeyNumpad0); // This overlaps with Insert
  SetKeyNameForScanCode(83, false, ezInputSlot_KeyNumpadPeriod); // This overlaps with Insert

  SetKeyNameForScanCode(86, false, ezInputSlot_KeyPipe);
  
  SetKeyNameForScanCode(87, false, "keyboard_f11");
  SetKeyNameForScanCode(88, false, "keyboard_f12");

  SetKeyNameForScanCode(91, true, ezInputSlot_KeyLeftWin);  // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(92, true, ezInputSlot_KeyRightWin); // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(93, true, ezInputSlot_KeyApps);     // Prints '' if it is not 'extended'

  // 'Extended' keys
  SetKeyNameForScanCode(28, true, ezInputSlot_KeyNumpadEnter);
  SetKeyNameForScanCode(29, true, ezInputSlot_KeyRightCtrl);
  SetKeyNameForScanCode(53, true, ezInputSlot_KeyNumpadSlash);
  SetKeyNameForScanCode(55, true, ezInputSlot_KeyPrint);
  SetKeyNameForScanCode(56, true, ezInputSlot_KeyRightAlt);
  SetKeyNameForScanCode(70, true, ezInputSlot_KeyPause);
  SetKeyNameForScanCode(71, true, ezInputSlot_KeyHome);
  SetKeyNameForScanCode(72, true, ezInputSlot_KeyUp);
  SetKeyNameForScanCode(73, true, ezInputSlot_KeyPageUp);

  SetKeyNameForScanCode(75, true, ezInputSlot_KeyLeft);
  SetKeyNameForScanCode(77, true, ezInputSlot_KeyRight);

  SetKeyNameForScanCode(79, true, ezInputSlot_KeyEnd);
  SetKeyNameForScanCode(80, true, ezInputSlot_KeyDown);
  SetKeyNameForScanCode(81, true, ezInputSlot_KeyPageDown);
  SetKeyNameForScanCode(82, true, ezInputSlot_KeyInsert);
  SetKeyNameForScanCode(83, true, ezInputSlot_KeyDelete);
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;
  ShowCursor(m_bShowCursor);
}

bool ezStandardInputDevice::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

