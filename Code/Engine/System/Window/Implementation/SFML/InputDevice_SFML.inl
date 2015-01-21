#include <System/PCH.h>
#include <System/Window/Implementation/SFML/InputDevice_SFML.h>
#include <SFML/Window.hpp>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStandardInputDevice, ezInputDeviceMouseKeyboard, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

bool ezStandardInputDevice::s_bMainWindowUsed = false;

ezStandardInputDevice::ezStandardInputDevice(sf::Window* pWindow, ezUInt32 uiWindowNumber)
{
  m_pWindow = pWindow;
  m_uiWindowNumber = uiWindowNumber;
  m_vLastMousePos.SetZero();

  m_bShowCursor = true;
  m_bClipCursor = false;

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

void ezStandardInputDevice::RegisterInputSlots()
{
  RegisterInputSlot(ezInputSlot_KeyLeft,  "Left",   ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRight, "Right",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyUp,    "Up",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyDown,  "Down",   ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyEscape,    "Escape",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeySpace,     "Space",      ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBackspace, "Backspace",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyReturn,    "Return",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyTab,       "Tab",        ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftShift,   "Left Shift",   ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightShift,  "Right Shift",  ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftCtrl,    "Left Ctrl",    ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightCtrl,   "Right Ctrl",   ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftAlt,     "Left Alt",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightAlt,    "Right Alt",    ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyLeftWin,     "Left Win",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyRightWin,    "Right Win",    ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyBracketOpen,   "[", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBracketClose,  "]", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeySemicolon,   ";",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyApostrophe,  "'",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeySlash,       "/",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyEquals,      "=",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyTilde,       "~",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyHyphen,      "-",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyComma,       ",",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPeriod,      ".",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyBackslash,   "\\", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyPipe,        "|",  ezInputSlotFlags::IsButton); // Same as \ in SFML

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

  RegisterInputSlot(ezInputSlot_KeyF1,  "F1",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF2,  "F2",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF3,  "F3",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF4,  "F4",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF5,  "F5",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF6,  "F6",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF7,  "F7",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF8,  "F8",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF9,  "F9",  ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF10, "F10", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF11, "F11", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyF12, "F12", ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_KeyHome,      "Home",       ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyEnd,       "End",        ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyDelete,    "Delete",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyInsert,    "Insert",     ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPageUp,    "Page Up",    ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPageDown,  "Page Down",  ezInputSlotFlags::IsButton);

  //RegisterInputSlot(ezInputSlot_KeyNumLock,       "Numlock",  ezInputSlotFlags::IsButton); // Not supported
  RegisterInputSlot(ezInputSlot_KeyNumpadPlus,    "Numpad +", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadMinus,   "Numpad -", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadStar,    "Numpad *", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyNumpadSlash,   "Numpad /", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyNumpadPeriod,  "Numpad .", ezInputSlotFlags::IsButton); // Not supported
  //RegisterInputSlot(ezInputSlot_KeyNumpadEnter,   "Enter",    ezInputSlotFlags::IsButton); // Same as Return in SFML

  // Not supported
  //RegisterInputSlot(ezInputSlot_KeyCapsLock,  "Capslock",     ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyPrint,     "Print",        ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyScroll,    "Scroll",       ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyPause,     "Pause",        ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_KeyApps,      "Application",  ezInputSlotFlags::IsButton);
  
  // Not supported
  //RegisterInputSlot(ezInputSlot_KeyPrevTrack,   "Previous Track", ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyNextTrack,   "Next Track",     ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyPlayPause,   "Play / Pause",   ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyStop,        "Stop",           ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyVolumeUp,    "Volume Up",      ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyVolumeDown,  "Volume Down",    ezInputSlotFlags::IsButton);
  //RegisterInputSlot(ezInputSlot_KeyMute,        "Mute",           ezInputSlotFlags::IsButton);

  RegisterInputSlot(ezInputSlot_MouseWheelUp,   "Mousewheel Up",    ezInputSlotFlags::IsMouseWheel);
  RegisterInputSlot(ezInputSlot_MouseWheelDown, "Mousewheel Down",  ezInputSlotFlags::IsMouseWheel);

  RegisterInputSlot(ezInputSlot_MouseMoveNegX, "Mouse Move Left",   ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosX, "Mouse Move Right",  ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMoveNegY, "Mouse Move Down",   ezInputSlotFlags::IsMouseAxisMove);
  RegisterInputSlot(ezInputSlot_MouseMovePosY, "Mouse Move Up",     ezInputSlotFlags::IsMouseAxisMove);

  RegisterInputSlot(ezInputSlot_MouseButton0, "Mousebutton 0", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton1, "Mousebutton 1", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton2, "Mousebutton 2", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton3, "Mousebutton 3", ezInputSlotFlags::IsButton);
  RegisterInputSlot(ezInputSlot_MouseButton4, "Mousebutton 4", ezInputSlotFlags::IsButton);

  // Not supported
  //RegisterInputSlot(ezInputSlot_MouseDblClick0, "Left Double Click",   ezInputSlotFlags::IsDoubleClick);
  //RegisterInputSlot(ezInputSlot_MouseDblClick1, "Right Double Click",  ezInputSlotFlags::IsDoubleClick);
  //RegisterInputSlot(ezInputSlot_MouseDblClick2, "Middle Double Click", ezInputSlotFlags::IsDoubleClick);

  RegisterInputSlot(ezInputSlot_MousePositionX, "Mouse Position X", ezInputSlotFlags::IsMouseAxisPosition);
  RegisterInputSlot(ezInputSlot_MousePositionY, "Mouse Position Y", ezInputSlotFlags::IsMouseAxisPosition);

  // Not supported
  //RegisterInputSlot(ezInputSlot_TouchPoint0,            "Touchpoint 1",             ezInputSlotFlags::IsTouchPoint);
  //RegisterInputSlot(ezInputSlot_TouchPoint0_PositionX,  "Touchpoint 1 Position X",  ezInputSlotFlags::IsTouchPosition);
  //RegisterInputSlot(ezInputSlot_TouchPoint0_PositionY,  "Touchpoint 1 Position Y",  ezInputSlotFlags::IsTouchPosition);
  // ...
}

inline float ToF(bool b)
{
  return b ? 1.0f : 0.0f;
}

void ezStandardInputDevice::UpdateInputSlotValues()
{
  m_InputSlotValues[ezInputSlot_KeyA]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::A));
  m_InputSlotValues[ezInputSlot_KeyB]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::B));
  m_InputSlotValues[ezInputSlot_KeyC]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::C));
  m_InputSlotValues[ezInputSlot_KeyD]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::D));
  m_InputSlotValues[ezInputSlot_KeyE]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::E));
  m_InputSlotValues[ezInputSlot_KeyF]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F));
  m_InputSlotValues[ezInputSlot_KeyG]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::G));
  m_InputSlotValues[ezInputSlot_KeyH]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::H));
  m_InputSlotValues[ezInputSlot_KeyI]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::I));
  m_InputSlotValues[ezInputSlot_KeyJ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::J));
  m_InputSlotValues[ezInputSlot_KeyK]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::K));
  m_InputSlotValues[ezInputSlot_KeyL]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::L));
  m_InputSlotValues[ezInputSlot_KeyM]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::M));
  m_InputSlotValues[ezInputSlot_KeyN]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::N));
  m_InputSlotValues[ezInputSlot_KeyO]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::O));
  m_InputSlotValues[ezInputSlot_KeyP]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::P));
  m_InputSlotValues[ezInputSlot_KeyQ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Q));
  m_InputSlotValues[ezInputSlot_KeyR]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::R));
  m_InputSlotValues[ezInputSlot_KeyS]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::S));
  m_InputSlotValues[ezInputSlot_KeyT]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::T));
  m_InputSlotValues[ezInputSlot_KeyU]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::U));
  m_InputSlotValues[ezInputSlot_KeyV]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::V));
  m_InputSlotValues[ezInputSlot_KeyW]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::W));
  m_InputSlotValues[ezInputSlot_KeyX]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::X));
  m_InputSlotValues[ezInputSlot_KeyY]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Y));
  m_InputSlotValues[ezInputSlot_KeyZ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Z));

  m_InputSlotValues[ezInputSlot_Key0]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num0));
  m_InputSlotValues[ezInputSlot_Key1]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num1));
  m_InputSlotValues[ezInputSlot_Key2]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num2));
  m_InputSlotValues[ezInputSlot_Key3]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num3));
  m_InputSlotValues[ezInputSlot_Key4]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num4));
  m_InputSlotValues[ezInputSlot_Key5]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num5));
  m_InputSlotValues[ezInputSlot_Key6]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num6));
  m_InputSlotValues[ezInputSlot_Key7]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num7));
  m_InputSlotValues[ezInputSlot_Key8]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num8));
  m_InputSlotValues[ezInputSlot_Key9]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Num9));

  m_InputSlotValues[ezInputSlot_KeyEscape]      = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape));
  m_InputSlotValues[ezInputSlot_KeyLeftCtrl]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
  m_InputSlotValues[ezInputSlot_KeyLeftShift]   = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift));
  m_InputSlotValues[ezInputSlot_KeyLeftAlt]     = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt));
  m_InputSlotValues[ezInputSlot_KeyLeftWin]     = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::LSystem));
  m_InputSlotValues[ezInputSlot_KeyRightCtrl]   = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::RControl));
  m_InputSlotValues[ezInputSlot_KeyRightShift]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::RShift));
  m_InputSlotValues[ezInputSlot_KeyRightAlt]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt));
  m_InputSlotValues[ezInputSlot_KeyRightWin]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::RSystem));
  m_InputSlotValues[ezInputSlot_KeyApps]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Menu));

  m_InputSlotValues[ezInputSlot_KeyBracketOpen]   = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::LBracket));
  m_InputSlotValues[ezInputSlot_KeyBracketClose]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::RBracket));
  m_InputSlotValues[ezInputSlot_KeySemicolon]     = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::SemiColon));
  m_InputSlotValues[ezInputSlot_KeyComma]         = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Comma));
  m_InputSlotValues[ezInputSlot_KeyPeriod]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Period));
  m_InputSlotValues[ezInputSlot_KeyApostrophe]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Quote));
  m_InputSlotValues[ezInputSlot_KeySlash]         = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Slash));
  m_InputSlotValues[ezInputSlot_KeyBackslash]     = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::BackSlash));
  m_InputSlotValues[ezInputSlot_KeyTilde]         = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Tilde));
  m_InputSlotValues[ezInputSlot_KeyEquals]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Equal));
  m_InputSlotValues[ezInputSlot_KeyHyphen]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Dash));
  m_InputSlotValues[ezInputSlot_KeySpace]         = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Space));
  m_InputSlotValues[ezInputSlot_KeyReturn]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Return));
  m_InputSlotValues[ezInputSlot_KeyBackspace]     = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::BackSpace));
  m_InputSlotValues[ezInputSlot_KeyTab]           = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Tab));

  m_InputSlotValues[ezInputSlot_KeyPageUp]      = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp));
  m_InputSlotValues[ezInputSlot_KeyPageDown]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown));
  m_InputSlotValues[ezInputSlot_KeyEnd]         = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::End));
  m_InputSlotValues[ezInputSlot_KeyHome]        = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Home));
  m_InputSlotValues[ezInputSlot_KeyInsert]      = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Insert));
  m_InputSlotValues[ezInputSlot_KeyDelete]      = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Delete));

  m_InputSlotValues[ezInputSlot_KeyNumpadPlus]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Add));
  m_InputSlotValues[ezInputSlot_KeyNumpadMinus]   = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Subtract));
  m_InputSlotValues[ezInputSlot_KeyNumpadStar]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Multiply));
  m_InputSlotValues[ezInputSlot_KeyNumpadSlash]   = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Divide));

  m_InputSlotValues[ezInputSlot_KeyLeft]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Left));
  m_InputSlotValues[ezInputSlot_KeyRight] = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Right));
  m_InputSlotValues[ezInputSlot_KeyUp]    = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Up));
  m_InputSlotValues[ezInputSlot_KeyDown]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Down));

  m_InputSlotValues[ezInputSlot_KeyNumpad0]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad0));
  m_InputSlotValues[ezInputSlot_KeyNumpad1]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad1));
  m_InputSlotValues[ezInputSlot_KeyNumpad2]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad2));
  m_InputSlotValues[ezInputSlot_KeyNumpad3]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad3));
  m_InputSlotValues[ezInputSlot_KeyNumpad4]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad4));
  m_InputSlotValues[ezInputSlot_KeyNumpad5]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad5));
  m_InputSlotValues[ezInputSlot_KeyNumpad6]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad6));
  m_InputSlotValues[ezInputSlot_KeyNumpad7]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad7));
  m_InputSlotValues[ezInputSlot_KeyNumpad8]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad8));
  m_InputSlotValues[ezInputSlot_KeyNumpad9]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad9));

  m_InputSlotValues[ezInputSlot_KeyF1 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F1 ));
  m_InputSlotValues[ezInputSlot_KeyF2 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F2 ));
  m_InputSlotValues[ezInputSlot_KeyF3 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F3 ));
  m_InputSlotValues[ezInputSlot_KeyF4 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F4 ));
  m_InputSlotValues[ezInputSlot_KeyF5 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F5 ));
  m_InputSlotValues[ezInputSlot_KeyF6 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F6 ));
  m_InputSlotValues[ezInputSlot_KeyF7 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F7 ));
  m_InputSlotValues[ezInputSlot_KeyF8 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F8 ));
  m_InputSlotValues[ezInputSlot_KeyF9 ]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F9 ));
  m_InputSlotValues[ezInputSlot_KeyF10]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F10));
  m_InputSlotValues[ezInputSlot_KeyF11]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F11));
  m_InputSlotValues[ezInputSlot_KeyF12]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::F12));

  m_InputSlotValues[ezInputSlot_KeyPause]  = ToF(sf::Keyboard::isKeyPressed(sf::Keyboard::Pause));

  m_InputSlotValues[ezInputSlot_MouseButton0] = ToF(sf::Mouse::isButtonPressed(sf::Mouse::Left));
  m_InputSlotValues[ezInputSlot_MouseButton1] = ToF(sf::Mouse::isButtonPressed(sf::Mouse::Right));
  m_InputSlotValues[ezInputSlot_MouseButton2] = ToF(sf::Mouse::isButtonPressed(sf::Mouse::Middle));
  m_InputSlotValues[ezInputSlot_MouseButton3] = ToF(sf::Mouse::isButtonPressed(sf::Mouse::XButton1));
  m_InputSlotValues[ezInputSlot_MouseButton4] = ToF(sf::Mouse::isButtonPressed(sf::Mouse::XButton2));
}

void ezStandardInputDevice::ResetInputSlotValues()
{
  m_InputSlotValues[ezInputSlot_MouseWheelUp]  = 0;
  m_InputSlotValues[ezInputSlot_MouseWheelDown]= 0;

  m_InputSlotValues[ezInputSlot_MouseMoveNegX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosX] = 0;
  m_InputSlotValues[ezInputSlot_MouseMoveNegY] = 0;
  m_InputSlotValues[ezInputSlot_MouseMovePosY] = 0;

  // Not supported
  //m_InputSlotValues[ezInputSlot_MouseDblClick0] = 0;
  //m_InputSlotValues[ezInputSlot_MouseDblClick1] = 0;
  //m_InputSlotValues[ezInputSlot_MouseDblClick2] = 0;
}


void ezStandardInputDevice::WindowMessage(const sf::Event& TheEvent)
{
  static bool s_bHasFocus = true;

  switch (TheEvent.type)
  {
  case sf::Event::TextEntered:
    m_LastCharacter = TheEvent.text.unicode;
    break;

  case sf::Event::LostFocus:
    s_bHasFocus = false;

    if (!m_bShowCursor)
    {
      m_pWindow->setMouseCursorVisible(true);

      sf::Vector2i iCurPos = sf::Mouse::getPosition(*m_pWindow);

      // if we lost focus, but the global mouse position is still inside the window (task switch), set the mouse position properly
      if (iCurPos.x > 0 && iCurPos.y > 0 && iCurPos.x < (int) m_pWindow->getSize().x && iCurPos.y < (int) m_pWindow->getSize().y)
      {
        // apply our emulated position to the actual mouse cursor, so that it will show up at the same position
        sf::Mouse::setPosition(sf::Vector2i(m_vEmulatedMousePos.x, m_vEmulatedMousePos.y), *m_pWindow);
      }
    }
    break;

  case sf::Event::GainedFocus:
    s_bHasFocus = true;

    if (!m_bShowCursor)
      m_pWindow->setMouseCursorVisible(false);

    m_vLastMousePos.Set(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y); // this ensures that the mouse diff is zero in the update step
    m_vEmulatedMousePos.Set(sf::Mouse::getPosition(*m_pWindow).x, sf::Mouse::getPosition(*m_pWindow).y); // just copy the new mouse position into the emulated position

    UpdateMouseCursor();
    break;

  case sf::Event::MouseWheelMoved:
    {
      if (TheEvent.mouseWheel.delta > 0)
        m_InputSlotValues[ezInputSlot_MouseWheelUp]   = (float)  TheEvent.mouseWheel.delta;

      if (TheEvent.mouseWheel.delta < 0)
        m_InputSlotValues[ezInputSlot_MouseWheelDown] = (float) -TheEvent.mouseWheel.delta;
    }
    break;

  case sf::Event::MouseLeft:
    {
      if (m_bClipCursor && s_bHasFocus)
        UpdateMouseCursor(); // this will re-center the mouse into the window
    }
    break;

  case sf::Event::MouseMoved:
    if (!s_bHasFocus)
      break;

    UpdateMouseCursor();
    break;

  default:
    // Nobody really knows what happened during THE EVENT.
    break;
  }
}

void ezStandardInputDevice::UpdateMouseCursor()
{
  if (m_vLastMousePos.IsZero())
    m_vLastMousePos.Set(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y);

  ezVec2I32 vDiff (sf::Mouse::getPosition().x - m_vLastMousePos.x, sf::Mouse::getPosition().y - m_vLastMousePos.y);

  // if the mouse cursor is hidden, we track both 'absolute' and relative mouse position, by emulating the absolute mouse position
  if (m_bClipCursor)
  {
    // re-center the mouse in the window
    if (!vDiff.IsZero())
      sf::Mouse::setPosition(sf::Vector2i(m_pWindow->getSize().x / 2, m_pWindow->getSize().y / 2), *m_pWindow);

    // compute our own mouse position out of the movement deltas
    m_vEmulatedMousePos += vDiff;
    m_vEmulatedMousePos.x = ezMath::Clamp<ezInt32>(m_vEmulatedMousePos.x, 0, m_pWindow->getSize().x - 1);
    m_vEmulatedMousePos.y = ezMath::Clamp<ezInt32>(m_vEmulatedMousePos.y, 0, m_pWindow->getSize().y - 1);

    m_InputSlotValues[ezInputSlot_MousePositionX] = ((float) m_vEmulatedMousePos.x / (float) m_pWindow->getSize().x) + m_uiWindowNumber;
    m_InputSlotValues[ezInputSlot_MousePositionY] = ((float) m_vEmulatedMousePos.y / (float) m_pWindow->getSize().y);
  }
  else
  {
    // if the cursor is shown, store the current mouse position, just to track it
    m_vEmulatedMousePos.Set(sf::Mouse::getPosition(*m_pWindow).x, sf::Mouse::getPosition(*m_pWindow).y);

    // also store the absolute mouse position
    m_InputSlotValues[ezInputSlot_MousePositionX] = ((float) m_vEmulatedMousePos.x / (float) m_pWindow->getSize().x) + m_uiWindowNumber;
    m_InputSlotValues[ezInputSlot_MousePositionY] = ((float) m_vEmulatedMousePos.y / (float) m_pWindow->getSize().y);
  }

  // store the new mouse position as the current one
  m_vLastMousePos.Set(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y);

  m_InputSlotValues[ezInputSlot_MouseMoveNegX] += ((vDiff.x < 0) ? (float) -vDiff.x : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[ezInputSlot_MouseMovePosX] += ((vDiff.x > 0) ? (float)  vDiff.x : 0.0f) * GetMouseSpeed().x;
  m_InputSlotValues[ezInputSlot_MouseMoveNegY] += ((vDiff.y < 0) ? (float) -vDiff.y : 0.0f) * GetMouseSpeed().y;
  m_InputSlotValues[ezInputSlot_MouseMovePosY] += ((vDiff.y > 0) ? (float)  vDiff.y : 0.0f) * GetMouseSpeed().y;
}

void ezStandardInputDevice::SetClipMouseCursor(bool bEnable)
{
  if (m_bClipCursor == bEnable)
    return;

  m_bClipCursor = bEnable;

  if (!m_bClipCursor)
  {
    // pass the emulated mouse position back to the OS, to prevent mouse jumping
    sf::Mouse::setPosition(sf::Vector2i(m_vEmulatedMousePos.x, m_vEmulatedMousePos.y), *m_pWindow);
  }
  else
  {
    // re-center the mouse cursor in the window
    sf::Mouse::setPosition(sf::Vector2i(m_pWindow->getSize().x / 2, m_pWindow->getSize().y / 2), *m_pWindow);
  }

  m_vLastMousePos.Set(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y);
}

void ezStandardInputDevice::SetShowMouseCursor(bool bShow)
{
  if (m_bShowCursor == bShow)
    return;

  m_bShowCursor = bShow;
  m_pWindow->setMouseCursorVisible(m_bShowCursor);
}





