#include <Core/PCH.h>
#include <Core/Input/InputDevice.h>
#include <Foundation/Containers/DynamicArray.h>

EZ_CORE_DLL const char* ConvertScanCodeToEngineName(ezUInt8 uiScanCode, bool bIsExtendedKey)
{
  const ezUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
  case 1:   return "keyboard_escape";
  case 2:   return "keyboard_1";
  case 3:   return "keyboard_2";
  case 4:   return "keyboard_3";
  case 5:   return "keyboard_4";
  case 6:   return "keyboard_5";
  case 7:   return "keyboard_6";
  case 8:   return "keyboard_7";
  case 9:   return "keyboard_8";
  case 10:  return "keyboard_9";
  case 11:  return "keyboard_0";
  case 12:  return "keyboard_hyphen";
  case 13:  return "keyboard_equals";
  case 14:  return "keyboard_backspace";
  case 15:  return "keyboard_tab";
  case 16:  return "keyboard_q";
  case 17:  return "keyboard_w";
  case 18:  return "keyboard_e";
  case 19:  return "keyboard_r";
  case 20:  return "keyboard_t";
  case 21:  return "keyboard_y";
  case 22:  return "keyboard_u";
  case 23:  return "keyboard_i";
  case 24:  return "keyboard_o";
  case 25:  return "keyboard_p";
  case 26:  return "keyboard_bracket_open";
  case 27:  return "keyboard_bracket_close";
  case 28:  return "keyboard_return";
  case 29:  return "keyboard_left_ctrl";
  case 30:  return "keyboard_a";
  case 31:  return "keyboard_s";
  case 32:  return "keyboard_d";
  case 33:  return "keyboard_f";
  case 34:  return "keyboard_g";
  case 35:  return "keyboard_h";
  case 36:  return "keyboard_j";
  case 37:  return "keyboard_k";
  case 38:  return "keyboard_l";
  case 39:  return "keyboard_semicolon";
  case 40:  return "keyboard_apostrophe";
  case 41:  return "keyboard_tilde";
  case 42:  return "keyboard_left_shift";
  case 43:  return "keyboard_backslash";
  case 44:  return "keyboard_z";
  case 45:  return "keyboard_x";
  case 46:  return "keyboard_c";
  case 47:  return "keyboard_v";
  case 48:  return "keyboard_b";
  case 49:  return "keyboard_n";
  case 50:  return "keyboard_m";
  case 51:  return "keyboard_comma";
  case 52:  return "keyboard_period";
  case 53:  return "keyboard_slash";
  case 54:  return "keyboard_right_shift";
  case 55:  return "keyboard_numpad_star";
  case 56:  return "keyboard_left_alt";
  case 57:  return "keyboard_space";
  case 58:  return "keyboard_capslock";
  case 59:  return "keyboard_f1";
  case 60:  return "keyboard_f2";
  case 61:  return "keyboard_f3";
  case 62:  return "keyboard_f4";
  case 63:  return "keyboard_f5";
  case 64:  return "keyboard_f6";
  case 65:  return "keyboard_f7";
  case 66:  return "keyboard_f8";
  case 67:  return "keyboard_f9";
  case 68:  return "keyboard_f10";
  case 69:  return "keyboard_numlock";
  case 70:  return "keyboard_scroll";
  case 71:  return "keyboard_numpad_7";
  case 72:  return "keyboard_numpad_8";
  case 73:  return "keyboard_numpad_9";
  case 74:  return "keyboard_numpad_minus";
  case 75:  return "keyboard_numpad_4";
  case 76:  return "keyboard_numpad_5";
  case 77:  return "keyboard_numpad_6";
  case 78:  return "keyboard_numpad_plus";
  case 79:  return "keyboard_numpad_1";
  case 80:  return "keyboard_numpad_2";
  case 81:  return "keyboard_numpad_3";
  case 82:  return "keyboard_numpad_0";
  case 83:  return "keyboard_numpad_period";


  case 86:  return "keyboard_pipe";
  case 87:  return "keyboard_f11";
  case 88:  return "keyboard_f12";


  case 91:  return "keyboard_left_win";
  case 92:  return "keyboard_right_win";
  case 93:  return "keyboard_apps";



  case 128 + 16:  return "keyboard_prev_track";
  case 128 + 25:  return "keyboard_next_track";
  case 128 + 28:  return "keyboard_numpad_enter";
  case 128 + 29:  return "keyboard_right_ctrl";
  case 128 + 32:  return "keyboard_mute";
  case 128 + 34:  return "keyboard_play_pause";
  case 128 + 36:  return "keyboard_stop";
  case 128 + 46:  return "keyboard_volume_down";
  case 128 + 48:  return "keyboard_volume_up";
  case 128 + 53:  return "keyboard_numpad_slash";
  case 128 + 55:  return "keyboard_print";
  case 128 + 56:  return "keyboard_right_alt";
  case 128 + 70:  return "keyboard_pause";
  case 128 + 71:  return "keyboard_home";
  case 128 + 72:  return "keyboard_up";
  case 128 + 73:  return "keyboard_page_up";
  case 128 + 75:  return "keyboard_left";
  case 128 + 77:  return "keyboard_right";
  case 128 + 79:  return "keyboard_end";
  case 128 + 80:  return "keyboard_down";
  case 128 + 81:  return "keyboard_page_down";
  case 128 + 82:  return "keyboard_insert";
  case 128 + 83:  return "keyboard_delete";

  default:

    // for extended keys fall back to the non-extended name
    if (bIsExtendedKey)
      return ConvertScanCodeToEngineName(uiScanCode, false);

    break;
  }

  return "unknown_key";
}



