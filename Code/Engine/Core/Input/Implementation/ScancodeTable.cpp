#include <Core/PCH.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Containers/DynamicArray.h>

const char* ezInputManager::ConvertScanCodeToEngineName(ezUInt8 uiScanCode, bool bIsExtendedKey)
{
  const ezUInt8 uiFinalScanCode = bIsExtendedKey ? (uiScanCode + 128) : uiScanCode;

  switch (uiFinalScanCode)
  {
  case 1:   return ezInputSlot_KeyEscape;
  case 2:   return ezInputSlot_Key1;
  case 3:   return ezInputSlot_Key2;
  case 4:   return ezInputSlot_Key3;
  case 5:   return ezInputSlot_Key4;
  case 6:   return ezInputSlot_Key5;
  case 7:   return ezInputSlot_Key6;
  case 8:   return ezInputSlot_Key7;
  case 9:   return ezInputSlot_Key8;
  case 10:  return ezInputSlot_Key9;
  case 11:  return ezInputSlot_Key0;
  case 12:  return ezInputSlot_KeyHyphen;
  case 13:  return ezInputSlot_KeyEquals;
  case 14:  return ezInputSlot_KeyBackspace;
  case 15:  return ezInputSlot_KeyTab;
  case 16:  return ezInputSlot_KeyQ;
  case 17:  return ezInputSlot_KeyW;
  case 18:  return ezInputSlot_KeyE;
  case 19:  return ezInputSlot_KeyR;
  case 20:  return ezInputSlot_KeyT;
  case 21:  return ezInputSlot_KeyY;
  case 22:  return ezInputSlot_KeyU;
  case 23:  return ezInputSlot_KeyI;
  case 24:  return ezInputSlot_KeyO;
  case 25:  return ezInputSlot_KeyP;
  case 26:  return ezInputSlot_KeyBracketOpen;
  case 27:  return ezInputSlot_KeyBracketClose;
  case 28:  return ezInputSlot_KeyReturn;
  case 29:  return ezInputSlot_KeyLeftCtrl;
  case 30:  return ezInputSlot_KeyA;
  case 31:  return ezInputSlot_KeyS;
  case 32:  return ezInputSlot_KeyD;
  case 33:  return ezInputSlot_KeyF;
  case 34:  return ezInputSlot_KeyG;
  case 35:  return ezInputSlot_KeyH;
  case 36:  return ezInputSlot_KeyJ;
  case 37:  return ezInputSlot_KeyK;
  case 38:  return ezInputSlot_KeyL;
  case 39:  return ezInputSlot_KeySemicolon;
  case 40:  return ezInputSlot_KeyApostrophe;
  case 41:  return ezInputSlot_KeyTilde;
  case 42:  return ezInputSlot_KeyLeftShift;
  case 43:  return ezInputSlot_KeyBackslash;
  case 44:  return ezInputSlot_KeyZ;
  case 45:  return ezInputSlot_KeyX;
  case 46:  return ezInputSlot_KeyC;
  case 47:  return ezInputSlot_KeyV;
  case 48:  return ezInputSlot_KeyB;
  case 49:  return ezInputSlot_KeyN;
  case 50:  return ezInputSlot_KeyM;
  case 51:  return ezInputSlot_KeyComma;
  case 52:  return ezInputSlot_KeyPeriod;
  case 53:  return ezInputSlot_KeySlash;
  case 54:  return ezInputSlot_KeyRightShift;
  case 55:  return ezInputSlot_KeyNumpadStar;
  case 56:  return ezInputSlot_KeyLeftAlt;
  case 57:  return ezInputSlot_KeySpace;
  case 58:  return ezInputSlot_KeyCapsLock;
  case 59:  return ezInputSlot_KeyF1;
  case 60:  return ezInputSlot_KeyF2;
  case 61:  return ezInputSlot_KeyF3;
  case 62:  return ezInputSlot_KeyF4;
  case 63:  return ezInputSlot_KeyF5;
  case 64:  return ezInputSlot_KeyF6;
  case 65:  return ezInputSlot_KeyF7;
  case 66:  return ezInputSlot_KeyF8;
  case 67:  return ezInputSlot_KeyF9;
  case 68:  return ezInputSlot_KeyF10;
  case 69:  return ezInputSlot_KeyNumLock;
  case 70:  return ezInputSlot_KeyScroll;
  case 71:  return ezInputSlot_KeyNumpad7;
  case 72:  return ezInputSlot_KeyNumpad8;
  case 73:  return ezInputSlot_KeyNumpad9;
  case 74:  return ezInputSlot_KeyNumpadMinus;
  case 75:  return ezInputSlot_KeyNumpad4;
  case 76:  return ezInputSlot_KeyNumpad5;
  case 77:  return ezInputSlot_KeyNumpad6;
  case 78:  return ezInputSlot_KeyNumpadPlus;
  case 79:  return ezInputSlot_KeyNumpad1;
  case 80:  return ezInputSlot_KeyNumpad2;
  case 81:  return ezInputSlot_KeyNumpad3;
  case 82:  return ezInputSlot_KeyNumpad0;
  case 83:  return ezInputSlot_KeyNumpadPeriod;


  case 86:  return ezInputSlot_KeyPipe;
  case 87:  return ezInputSlot_KeyF11;
  case 88:  return ezInputSlot_KeyF12;


  case 91:  return ezInputSlot_KeyLeftWin;
  case 92:  return ezInputSlot_KeyRightWin;
  case 93:  return ezInputSlot_KeyApps;



  case 128 + 16:  return ezInputSlot_KeyPrevTrack;
  case 128 + 25:  return ezInputSlot_KeyNextTrack;
  case 128 + 28:  return ezInputSlot_KeyNumpadEnter;
  case 128 + 29:  return ezInputSlot_KeyRightCtrl;
  case 128 + 32:  return ezInputSlot_KeyMute;
  case 128 + 34:  return ezInputSlot_KeyPlayPause;
  case 128 + 36:  return ezInputSlot_KeyStop;
  case 128 + 46:  return ezInputSlot_KeyVolumeDown;
  case 128 + 48:  return ezInputSlot_KeyVolumeUp;
  case 128 + 53:  return ezInputSlot_KeyNumpadSlash;
  case 128 + 55:  return ezInputSlot_KeyPrint;
  case 128 + 56:  return ezInputSlot_KeyRightAlt;
  case 128 + 70:  return ezInputSlot_KeyPause;
  case 128 + 71:  return ezInputSlot_KeyHome;
  case 128 + 72:  return ezInputSlot_KeyUp;
  case 128 + 73:  return ezInputSlot_KeyPageUp;
  case 128 + 75:  return ezInputSlot_KeyLeft;
  case 128 + 77:  return ezInputSlot_KeyRight;
  case 128 + 79:  return ezInputSlot_KeyEnd;
  case 128 + 80:  return ezInputSlot_KeyDown;
  case 128 + 81:  return ezInputSlot_KeyPageDown;
  case 128 + 82:  return ezInputSlot_KeyInsert;
  case 128 + 83:  return ezInputSlot_KeyDelete;

  default:

    // for extended keys fall back to the non-extended name
    if (bIsExtendedKey)
      return ConvertScanCodeToEngineName(uiScanCode, false);

    break;
  }

  return "unknown_key";
}





EZ_STATICLINK_FILE(Core, Core_Input_Implementation_ScancodeTable);

