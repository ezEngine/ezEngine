#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <InputWindows/InputDeviceWindows.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Containers/HybridArray.h>

EZ_INPUTWINDOWS_DLL ezInputDeviceWindows g_InputDeviceWindows(0);

bool ezInputDeviceWindows::s_bMainWindowUsed = false;

ezInputDeviceWindows::ezInputDeviceWindows(ezUInt32 uiWindowNumber)
{
  m_uiWindowNumber = uiWindowNumber;
  m_bClipCursor = false;
  m_bShowCursor = true;

  if (uiWindowNumber == 0)
  {
    EZ_ASSERT_API(!s_bMainWindowUsed, "You cannot have two devices of Type ezInputDeviceWindows with the window number zero.");
    ezInputDeviceWindows::s_bMainWindowUsed = true;
  }
}

ezInputDeviceWindows::~ezInputDeviceWindows()
{
  if (m_uiWindowNumber == 0)
    ezInputDeviceWindows::s_bMainWindowUsed = false;
}

void ezInputDeviceWindows::InitializeDevice()
{
  if (m_uiWindowNumber == 0)
  {
    RAWINPUTDEVICE Rid[2];

    // keyboard
    Rid[0].usUsagePage = 0x01; 
    Rid[0].usUsage = 0x06; 
    Rid[0].dwFlags = RIDEV_NOHOTKEYS; // Disables Windows-Key and Application-Key
    Rid[0].hwndTarget = NULL;

    // mouse
    Rid[1].usUsagePage = 0x01; 
    Rid[1].usUsage = 0x02; 
    Rid[1].dwFlags = 0;
    Rid[1].hwndTarget = NULL;

    if (RegisterRawInputDevices(&Rid[0], (UINT) 2, sizeof(RAWINPUTDEVICE)) == FALSE) 
    {
      ezLog::Error("Could not initialize RawInput for Mouse and Keyboard input.");
    }
    else
      ezLog::Success("Initialized RawInput for Mouse and Keyboard input.");
  }
  else
    ezLog::Info("Window %i does not need to initialize Mouse or Keybard.", m_uiWindowNumber);
}

void ezInputDeviceWindows::RegisterInputSlots()
{
  RegisterInputSlot("keyboard_left", "Left");
  RegisterInputSlot("keyboard_right", "Right");
  RegisterInputSlot("keyboard_up", "Up");
  RegisterInputSlot("keyboard_down", "Down");

  RegisterInputSlot("keyboard_escape", "Escape");
  RegisterInputSlot("keyboard_space", "Space");
  RegisterInputSlot("keyboard_backspace", "Backspace");
  RegisterInputSlot("keyboard_return", "Return");
  RegisterInputSlot("keyboard_tab", "Tab");

  RegisterInputSlot("keyboard_left_shift", "Left Shift");
  RegisterInputSlot("keyboard_right_shift", "Right Shift");

  RegisterInputSlot("keyboard_left_ctrl", "Left Ctrl");
  RegisterInputSlot("keyboard_right_ctrl", "Right Ctrl");

  RegisterInputSlot("keyboard_left_alt", "Left Alt");
  RegisterInputSlot("keyboard_right_alt", "Right Alt");

  RegisterInputSlot("keyboard_left_win", "Left Win");
  RegisterInputSlot("keyboard_right_win", "Right Win");

  RegisterInputSlot("keyboard_bracket_open", "[");
  RegisterInputSlot("keyboard_bracket_close", "]");

  RegisterInputSlot("keyboard_semicolon", ";");
  RegisterInputSlot("keyboard_apostrophe", "'");
  RegisterInputSlot("keyboard_slash", "/");
  RegisterInputSlot("keyboard_equals", "=");
  RegisterInputSlot("keyboard_tilde", "~");
  RegisterInputSlot("keyboard_hyphen", "-");
  RegisterInputSlot("keyboard_comma", ",");
  RegisterInputSlot("keyboard_period", ".");
  RegisterInputSlot("keyboard_backslash", "\\");
  RegisterInputSlot("keyboard_pipe", "|");

  RegisterInputSlot("keyboard_1", "1");
  RegisterInputSlot("keyboard_2", "2");
  RegisterInputSlot("keyboard_3", "3");
  RegisterInputSlot("keyboard_4", "4");
  RegisterInputSlot("keyboard_5", "5");
  RegisterInputSlot("keyboard_6", "6");
  RegisterInputSlot("keyboard_7", "7");
  RegisterInputSlot("keyboard_8", "8");
  RegisterInputSlot("keyboard_9", "9");
  RegisterInputSlot("keyboard_0", "0");

  RegisterInputSlot("keyboard_numpad_1", "Numpad 1");
  RegisterInputSlot("keyboard_numpad_2", "Numpad 2");
  RegisterInputSlot("keyboard_numpad_3", "Numpad 3");
  RegisterInputSlot("keyboard_numpad_4", "Numpad 4");
  RegisterInputSlot("keyboard_numpad_5", "Numpad 5");
  RegisterInputSlot("keyboard_numpad_6", "Numpad 6");
  RegisterInputSlot("keyboard_numpad_7", "Numpad 7");
  RegisterInputSlot("keyboard_numpad_8", "Numpad 8");
  RegisterInputSlot("keyboard_numpad_9", "Numpad 9");
  RegisterInputSlot("keyboard_numpad_0", "Numpad 0");

  RegisterInputSlot("keyboard_a", "A");
  RegisterInputSlot("keyboard_b", "B");
  RegisterInputSlot("keyboard_c", "C");
  RegisterInputSlot("keyboard_d", "D");
  RegisterInputSlot("keyboard_e", "E");
  RegisterInputSlot("keyboard_f", "F");
  RegisterInputSlot("keyboard_g", "G");
  RegisterInputSlot("keyboard_h", "H");
  RegisterInputSlot("keyboard_i", "I");
  RegisterInputSlot("keyboard_j", "J");
  RegisterInputSlot("keyboard_k", "K");
  RegisterInputSlot("keyboard_l", "L");
  RegisterInputSlot("keyboard_m", "M");
  RegisterInputSlot("keyboard_n", "N");
  RegisterInputSlot("keyboard_o", "O");
  RegisterInputSlot("keyboard_p", "P");
  RegisterInputSlot("keyboard_q", "Q");
  RegisterInputSlot("keyboard_r", "R");
  RegisterInputSlot("keyboard_s", "S");
  RegisterInputSlot("keyboard_t", "T");
  RegisterInputSlot("keyboard_u", "U");
  RegisterInputSlot("keyboard_v", "V");
  RegisterInputSlot("keyboard_w", "W");
  RegisterInputSlot("keyboard_x", "X");
  RegisterInputSlot("keyboard_y", "Y");
  RegisterInputSlot("keyboard_z", "Z");

  RegisterInputSlot("keyboard_f1", "F1");
  RegisterInputSlot("keyboard_f2", "F2");
  RegisterInputSlot("keyboard_f3", "F3");
  RegisterInputSlot("keyboard_f4", "F4");
  RegisterInputSlot("keyboard_f5", "F5");
  RegisterInputSlot("keyboard_f6", "F6");
  RegisterInputSlot("keyboard_f7", "F7");
  RegisterInputSlot("keyboard_f8", "F8");
  RegisterInputSlot("keyboard_f9", "F9");
  RegisterInputSlot("keyboard_f10", "F10");
  RegisterInputSlot("keyboard_f11", "F11");
  RegisterInputSlot("keyboard_f12", "F12");

  RegisterInputSlot("keyboard_home", "Home");
  RegisterInputSlot("keyboard_end", "End");
  RegisterInputSlot("keyboard_delete", "Delete");
  RegisterInputSlot("keyboard_insert", "Insert");
  RegisterInputSlot("keyboard_page_up", "Page Up");
  RegisterInputSlot("keyboard_page_down", "Page Down");

  RegisterInputSlot("keyboard_numlock", "Numlock");
  RegisterInputSlot("keyboard_numpad_plus", "Numpad +");
  RegisterInputSlot("keyboard_numpad_minus", "Numpad -");
  RegisterInputSlot("keyboard_numpad_star", "Numpad *");
  RegisterInputSlot("keyboard_numpad_slash", "Numpad /");
  RegisterInputSlot("keyboard_numpad_period", "Numpad .");
  RegisterInputSlot("keyboard_numpad_enter", "Enter");

  RegisterInputSlot("keyboard_capslock", "Capslock");
  RegisterInputSlot("keyboard_print", "Print");
  RegisterInputSlot("keyboard_scroll", "Scroll");
  RegisterInputSlot("keyboard_pause", "Pause");
  RegisterInputSlot("keyboard_apps", "Application");
  
  RegisterInputSlot("keyboard_prev_track", "Previous Track");
  RegisterInputSlot("keyboard_next_track", "Next Track");
  RegisterInputSlot("keyboard_play_pause", "Play / Pause");
  RegisterInputSlot("keyboard_stop", "Stop");
  RegisterInputSlot("keyboard_volume_up", "Volume Up");
  RegisterInputSlot("keyboard_volume_down", "Volume Down");
  RegisterInputSlot("keyboard_mute", "Mute");

  RegisterInputSlot("mouse_wheel_up", "Mousewheel Up");
  RegisterInputSlot("mouse_wheel_down", "Mousewheel Down");

  RegisterInputSlot("mouse_move_negx", "Mouse Move Left");
  RegisterInputSlot("mouse_move_posx", "Mouse Move Right");
  RegisterInputSlot("mouse_move_negy", "Mouse Move Down");
  RegisterInputSlot("mouse_move_posy", "Mouse Move Up");

  RegisterInputSlot("mouse_button_0", "Mousebutton 0");
  RegisterInputSlot("mouse_button_1", "Mousebutton 1");
  RegisterInputSlot("mouse_button_2", "Mousebutton 2");
  RegisterInputSlot("mouse_button_3", "Mousebutton 3");
  RegisterInputSlot("mouse_button_4", "Mousebutton 4");

  RegisterInputSlot("mouse_button_0_doubleclick", "Left Double Click");
  RegisterInputSlot("mouse_button_1_doubleclick", "Right Double Click");
  RegisterInputSlot("mouse_button_2_doubleclick", "Middle Double Click");

  // dead-zones on mice are a pretty bad idea, most mice only send deltas of 1, but they send them pretty often
  // instead scale the mouse input down, to reduce its impact
  ezInputManager::SetInputSlotDeadZone("mouse_move_negx", 0.0f);
  ezInputManager::SetInputSlotDeadZone("mouse_move_posx", 0.0f);
  ezInputManager::SetInputSlotDeadZone("mouse_move_negy", 0.0f);
  ezInputManager::SetInputSlotDeadZone("mouse_move_posy", 0.0f);

  ezInputManager::SetInputSlotScale("mouse_move_negx", 0.1f);
  ezInputManager::SetInputSlotScale("mouse_move_posx", 0.1f);
  ezInputManager::SetInputSlotScale("mouse_move_negy", 0.1f);
  ezInputManager::SetInputSlotScale("mouse_move_posy", 0.1f);


  RegisterInputSlot("mouse_position_x", "Mouse Position X");
  RegisterInputSlot("mouse_position_y", "Mouse Position Y");

  ezInputManager::SetInputSlotDeadZone("mouse_position_x", 0.0f);
  ezInputManager::SetInputSlotDeadZone("mouse_position_y", 0.0f);
}

void ezInputDeviceWindows::ResetInputSlotValues()
{
  m_InputSlotValues["mouse_wheel_up"]  = 0;
  m_InputSlotValues["mouse_wheel_down"]= 0;
  m_InputSlotValues["mouse_move_negx"] = 0;
  m_InputSlotValues["mouse_move_posx"] = 0;
  m_InputSlotValues["mouse_move_negy"] = 0;
  m_InputSlotValues["mouse_move_posy"] = 0;
  m_InputSlotValues["mouse_button_0_doubleclick"] = 0;
  m_InputSlotValues["mouse_button_1_doubleclick"] = 0;
  m_InputSlotValues["mouse_button_2_doubleclick"] = 0;
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
    ClipCursor(NULL);
}

void ezInputDeviceWindows::SetClipMouseCursor(bool bEnable)
{
  m_bClipCursor = bEnable;

  if (!bEnable)
    ClipCursor(NULL);
}


void ezInputDeviceWindows::WindowMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch (Msg)
  {
  case WM_MOUSEWHEEL:
    {
      // The mousewheel does not work with rawinput over touchpads (at least not all)
      // So we handle that one individually

      const ezInt32 iRotated = (ezInt16) HIWORD(wParam);

      if (iRotated > 0)
        m_InputSlotValues["mouse_wheel_up"]   =  iRotated /  120.0f;
      else
        m_InputSlotValues["mouse_wheel_down"] =  iRotated / -120.0f;
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

      m_InputSlotValues["mouse_position_x"] = (fPosX / uiResX) + m_uiWindowNumber;
      m_InputSlotValues["mouse_position_y"] = (fPosY / uiResY);


    }
    break;

  case WM_LBUTTONUP:
    {
      SetClipRect(m_bClipCursor, hWnd);
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

      ezMap<ezString, float, ezCompareHelper<ezString>, ezStaticAllocatorWrapper>::Iterator it = m_InputSlotValues.GetIterator();

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
    m_InputSlotValues["mouse_button_0_doubleclick"] = 1.0f;
    return;
  case WM_RBUTTONDBLCLK:
    m_InputSlotValues["mouse_button_1_doubleclick"] = 1.0f;
    return;
  case WM_MBUTTONDBLCLK:
    m_InputSlotValues["mouse_button_2_doubleclick"] = 1.0f;
    return;

  case WM_INPUT:
    {
      ezUInt32 uiSize;

      GetRawInputData((HRAWINPUT) lParam, RID_INPUT, NULL, &uiSize, sizeof(RAWINPUTHEADER));

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

        const char* szInputSlotName = ConvertScanCodeToEngineName(uiScanCode, bIsExtended);

        // On Windows this only happens with the Pause key, but it will actually send the 'Right Ctrl' key value
        // so we need to fix this manually
        if (raw->data.keyboard.Flags & RI_KEY_E1)
        {
          szInputSlotName = "keyboard_pause";
          bIgnoreNext = true;
        }

        // The Print key is sent as a two key sequence, first an 'extended left shift' and then the Numpad* key is sent
        // we ignore the first stupid shift key entirely and then modify the following Numpad* key
        // Note that the 'stupid shift' is sent along with several other keys as well (e.g. left/right/up/down arrows)
        // in these cases we can ignore them entirely, as the following key will have an unambiguous key code
        if (ezStringUtils::IsEqual(szInputSlotName, "keyboard_numpad_star") && bWasStupidLeftShift)
          szInputSlotName = "keyboard_print";

        bWasStupidLeftShift = false;

        int iRequest = raw->data.keyboard.MakeCode << 16;

        if (raw->data.keyboard.Flags & RI_KEY_E0) 
          iRequest |= 1 << 24;

        const bool bPressed = !(raw->data.keyboard.Flags & 0x01);

        m_InputSlotValues[szInputSlotName] = bPressed ? 1.0f : 0.0f;

        if ((m_InputSlotValues["keyboard_left_ctrl"] > 0.1f) && (m_InputSlotValues["keyboard_left_alt"] > 0.1f) && (m_InputSlotValues["keyboard_numpad_enter"] > 0.1f))
          SetClipMouseCursor(!m_bClipCursor);
      }
      else if (raw->header.dwType == RIM_TYPEMOUSE)
      {
        const ezUInt32 uiButtons = raw->data.mouse.usButtonFlags;

        // "absolute" positions are only reported by devices such as Pens
        // if at all, we should handle them as touch points, not as mouse positions
        if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
        {
          m_InputSlotValues["mouse_move_negx"] += (raw->data.mouse.lLastX < 0) ? (float) -raw->data.mouse.lLastX : 0.0f;
          m_InputSlotValues["mouse_move_posx"] += (raw->data.mouse.lLastX > 0) ? (float)  raw->data.mouse.lLastX : 0.0f;
          m_InputSlotValues["mouse_move_negy"] += (raw->data.mouse.lLastY < 0) ? (float) -raw->data.mouse.lLastY : 0.0f;
          m_InputSlotValues["mouse_move_posy"] += (raw->data.mouse.lLastY > 0) ? (float)  raw->data.mouse.lLastY : 0.0f;
        }
        //else
        //if (raw->data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE)
        //{
        //  ezLog::Info("Absolute Mouse Move: %.1f | %.1f", (float) raw->data.mouse.lLastX, (float) raw->data.mouse.lLastY);
        //}
        //else
        //if (raw->data.mouse.usFlags == MOUSE_VIRTUAL_DESKTOP)
        //{
        //  ezLog::Info("Virtual Mouse Move: %.1f | %.1f", (float) raw->data.mouse.lLastX, (float) raw->data.mouse.lLastY);
        //}
        //else
        //if (raw->data.mouse.usFlags == (MOUSE_VIRTUAL_DESKTOP | MOUSE_MOVE_ABSOLUTE))
        //{
        //  ezLog::Info("Virtual/Abs Mouse Move: %.1f | %.1f", (float) raw->data.mouse.lLastX, (float) raw->data.mouse.lLastY);
        //}
        //else
        //{
        //  ezLog::Info("Unknown Mouse Move: %.1f | %.1f, Flags = %i", (float) raw->data.mouse.lLastX, (float) raw->data.mouse.lLastY, raw->data.mouse.usFlags);
        //}


        for (ezInt32 mb = 0; mb < 5; ++mb)
        {
          char szTemp[32];
          ezStringUtils::snprintf(szTemp, 32, "mouse_button_%i", mb);

          if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2))) != 0)
            m_InputSlotValues[szTemp] = 1.0f;
          
          if ((uiButtons & (RI_MOUSE_BUTTON_1_DOWN << (mb * 2 + 1))) != 0)
            m_InputSlotValues[szTemp] = 0.0f;
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

void ezInputDeviceWindows::LocalizeButtonDisplayNames()
{
  EZ_LOG_BLOCK("ezInputDeviceWindows::LocalizeButtonDisplayNames");

  SetKeyNameForScanCode( 1, false, "keyboard_escape");
  SetKeyNameForScanCode( 2, false, "keyboard_1");
  SetKeyNameForScanCode( 3, false, "keyboard_2");
  SetKeyNameForScanCode( 4, false, "keyboard_3");
  SetKeyNameForScanCode( 5, false, "keyboard_4");
  SetKeyNameForScanCode( 6, false, "keyboard_5");
  SetKeyNameForScanCode( 7, false, "keyboard_6");
  SetKeyNameForScanCode( 8, false, "keyboard_7");
  SetKeyNameForScanCode( 9, false, "keyboard_8");
  SetKeyNameForScanCode(10, false, "keyboard_9");
  SetKeyNameForScanCode(11, false, "keyboard_0");

  SetKeyNameForScanCode(12, false, "keyboard_hyphen");
  SetKeyNameForScanCode(13, false, "keyboard_equals");
  SetKeyNameForScanCode(14, false, "keyboard_backspace");

  SetKeyNameForScanCode(15, false, "keyboard_tab");
  SetKeyNameForScanCode(16, false, "keyboard_q");
  SetKeyNameForScanCode(17, false, "keyboard_w");
  SetKeyNameForScanCode(18, false, "keyboard_e");
  SetKeyNameForScanCode(19, false, "keyboard_r");
  SetKeyNameForScanCode(20, false, "keyboard_t");
  SetKeyNameForScanCode(21, false, "keyboard_y");
  SetKeyNameForScanCode(22, false, "keyboard_u");
  SetKeyNameForScanCode(23, false, "keyboard_i");
  SetKeyNameForScanCode(24, false, "keyboard_o");
  SetKeyNameForScanCode(25, false, "keyboard_p");
  SetKeyNameForScanCode(26, false, "keyboard_bracket_open");
  SetKeyNameForScanCode(27, false, "keyboard_bracket_close");
  SetKeyNameForScanCode(28, false, "keyboard_return");

  SetKeyNameForScanCode(29, false, "keyboard_left_ctrl");
  SetKeyNameForScanCode(30, false, "keyboard_a");
  SetKeyNameForScanCode(31, false, "keyboard_s");
  SetKeyNameForScanCode(32, false, "keyboard_d");
  SetKeyNameForScanCode(33, false, "keyboard_f");
  SetKeyNameForScanCode(34, false, "keyboard_g");
  SetKeyNameForScanCode(35, false, "keyboard_h");
  SetKeyNameForScanCode(36, false, "keyboard_j");
  SetKeyNameForScanCode(37, false, "keyboard_k");
  SetKeyNameForScanCode(38, false, "keyboard_l");
  SetKeyNameForScanCode(39, false, "keyboard_semicolon");
  SetKeyNameForScanCode(40, false, "keyboard_apostrophe");

  SetKeyNameForScanCode(41, false, "keyboard_tilde");
  SetKeyNameForScanCode(42, false, "keyboard_left_shift");
  SetKeyNameForScanCode(43, false, "keyboard_backslash");

  SetKeyNameForScanCode(44, false, "keyboard_z");
  SetKeyNameForScanCode(45, false, "keyboard_x");
  SetKeyNameForScanCode(46, false, "keyboard_c");
  SetKeyNameForScanCode(47, false, "keyboard_v");
  SetKeyNameForScanCode(48, false, "keyboard_b");
  SetKeyNameForScanCode(49, false, "keyboard_n");
  SetKeyNameForScanCode(50, false, "keyboard_m");
  SetKeyNameForScanCode(51, false, "keyboard_comma");
  SetKeyNameForScanCode(52, false, "keyboard_period");
  SetKeyNameForScanCode(53, false, "keyboard_slash");
  SetKeyNameForScanCode(54, false, "keyboard_right_shift");
  
  SetKeyNameForScanCode(55, false, "keyboard_numpad_star"); // Overlaps with Print

  SetKeyNameForScanCode(56, false, "keyboard_left_alt");
  SetKeyNameForScanCode(57, false, "keyboard_space");
  SetKeyNameForScanCode(58, false, "keyboard_capslock");

  SetKeyNameForScanCode(59, false, "keyboard_f1");
  SetKeyNameForScanCode(60, false, "keyboard_f2");
  SetKeyNameForScanCode(61, false, "keyboard_f3");
  SetKeyNameForScanCode(62, false, "keyboard_f4");
  SetKeyNameForScanCode(63, false, "keyboard_f5");
  SetKeyNameForScanCode(64, false, "keyboard_f6");
  SetKeyNameForScanCode(65, false, "keyboard_f7");
  SetKeyNameForScanCode(66, false, "keyboard_f8");
  SetKeyNameForScanCode(67, false, "keyboard_f9");
  SetKeyNameForScanCode(68, false, "keyboard_f10");

  SetKeyNameForScanCode(69, true, "keyboard_numlock");  // Prints 'Pause' if it is not 'extended'
  SetKeyNameForScanCode(70, false, "keyboard_scroll");  // This overlaps with Pause

  SetKeyNameForScanCode(71, false, "keyboard_numpad_7"); // This overlaps with Home
  SetKeyNameForScanCode(72, false, "keyboard_numpad_8"); // This overlaps with Arrow Up
  SetKeyNameForScanCode(73, false, "keyboard_numpad_9"); // This overlaps with Page Up
  SetKeyNameForScanCode(74, false, "keyboard_numpad_minus");

  SetKeyNameForScanCode(75, false, "keyboard_numpad_4"); // This overlaps with Arrow Left
  SetKeyNameForScanCode(76, false, "keyboard_numpad_5");
  SetKeyNameForScanCode(77, false, "keyboard_numpad_6"); // This overlaps with Arrow Right
  SetKeyNameForScanCode(78, false, "keyboard_numpad_plus");

  SetKeyNameForScanCode(79, false, "keyboard_numpad_1"); // This overlaps with End
  SetKeyNameForScanCode(80, false, "keyboard_numpad_2"); // This overlaps with Arrow Down
  SetKeyNameForScanCode(81, false, "keyboard_numpad_3"); // This overlaps with Page Down
  SetKeyNameForScanCode(82, false, "keyboard_numpad_0"); // This overlaps with Insert
  SetKeyNameForScanCode(83, false, "keyboard_numpad_period"); // This overlaps with Insert

  SetKeyNameForScanCode(86, false, "keyboard_pipe");
  
  SetKeyNameForScanCode(87, false, "keyboard_f11");
  SetKeyNameForScanCode(88, false, "keyboard_f12");

  SetKeyNameForScanCode(91, true, "keyboard_left_win");  // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(92, true, "keyboard_right_win"); // Prints '' if it is not 'extended'
  SetKeyNameForScanCode(93, true, "keyboard_apps");     // Prints '' if it is not 'extended'

  // 'Extended' keys
  SetKeyNameForScanCode(28, true, "keyboard_numpad_enter");
  SetKeyNameForScanCode(29, true, "keyboard_right_ctrl");
  SetKeyNameForScanCode(53, true, "keyboard_numpad_slash");
  SetKeyNameForScanCode(55, true, "keyboard_print");
  SetKeyNameForScanCode(56, true, "keyboard_right_alt");
  SetKeyNameForScanCode(70, true, "keyboard_pause");
  SetKeyNameForScanCode(71, true, "keyboard_home");
  SetKeyNameForScanCode(72, true, "keyboard_up");
  SetKeyNameForScanCode(73, true, "keyboard_page_up");

  SetKeyNameForScanCode(75, true, "keyboard_left");
  SetKeyNameForScanCode(77, true, "keyboard_right");

  SetKeyNameForScanCode(79, true, "keyboard_end");
  SetKeyNameForScanCode(80, true, "keyboard_down");
  SetKeyNameForScanCode(81, true, "keyboard_page_down");
  SetKeyNameForScanCode(82, true, "keyboard_insert");
  SetKeyNameForScanCode(83, true, "keyboard_delete");

  // Doesn't work for these keys :(
  //SetKeyNameForScanCode(16, true, "keyboard_prev_track");
  //SetKeyNameForScanCode(25, true, "keyboard_next_track");
  //SetKeyNameForScanCode(34, true, "keyboard_play_pause");
  //SetKeyNameForScanCode(36, true, "keyboard_stop");
  //SetKeyNameForScanCode(46, true, "keyboard_volume_down");
  //SetKeyNameForScanCode(48, true, "keyboard_volume_up");
  //SetKeyNameForScanCode(32, true, "keyboard_mute");
}

void ezInputDeviceWindows::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;
  ShowCursor(m_bShowCursor);
}

bool ezInputDeviceWindows::GetShowMouseCursor() const
{
  return m_bShowCursor;
}

