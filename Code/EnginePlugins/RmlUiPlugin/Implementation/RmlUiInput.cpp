
#include <RmlUiPlugin/RmlUiInput.h>
#include <Core/Input/InputManager.h>


ezRmlUiInputSnapshot ezRmlUiInputSnapshot::MakeFromCurrentInput()
{
  ezRmlUiInputSnapshot snapshot = ezRmlUiInputSnapshot::MakeEmpty();

  const bool bCtrlPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftCtrl) >= ezKeyState::Pressed ||
    ezInputManager::GetInputSlotState(ezInputSlot_KeyRightCtrl) >= ezKeyState::Pressed;
  const bool bShiftPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftShift) >= ezKeyState::Pressed ||
    ezInputManager::GetInputSlotState(ezInputSlot_KeyRightShift) >= ezKeyState::Pressed;
  const bool bAltPressed = ezInputManager::GetInputSlotState(ezInputSlot_KeyLeftAlt) >= ezKeyState::Pressed ||
    ezInputManager::GetInputSlotState(ezInputSlot_KeyRightAlt) >= ezKeyState::Pressed;

  if (bCtrlPressed)
    snapshot.m_Buttons |= ezRmlUiInputButtons::Ctrl;
  if (bShiftPressed)
    snapshot.m_Buttons |= ezRmlUiInputButtons::Shift;
  if (bAltPressed)
    snapshot.m_Buttons |= ezRmlUiInputButtons::Alt;

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(ezRmlUiInputButtons::s_MouseButtonMappings); ++i)
  {
    ezRmlUiInputButtons::MouseButtonMapping mbm = ezRmlUiInputButtons::s_MouseButtonMappings[i];
    if (ezInputManager::GetInputSlotState(mbm.szEzButton) >= ezKeyState::Pressed)
      snapshot.m_Buttons |= mbm.uiEzButton;
  }

  if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelDown) == ezKeyState::Pressed)
  {
    snapshot.m_Buttons |= ezRmlUiInputButtons::MouseWheelDown;
  }
  if (ezInputManager::GetInputSlotState(ezInputSlot_MouseWheelUp) == ezKeyState::Pressed)
  {
    snapshot.m_Buttons |= ezRmlUiInputButtons::MouseWheelUp;
  }

  snapshot.m_uiLastCharacter = ezInputManager::RetrieveLastCharacter(false);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(ezRmlUiInputButtons::s_KeyMappings); ++i)
  {
    ezRmlUiInputButtons::KeyMapping km = ezRmlUiInputButtons::s_KeyMappings[i];
    if (ezInputManager::GetInputSlotState(km.szEzKey) >= ezKeyState::Pressed)
      snapshot.m_Buttons |= km.uiEzKey;
  }

  return snapshot;
}


bool ezRmlUiInputProvider::Update(ezRmlUiInputSnapshot input)
{
  bool bHasChanged = m_Buttons != input.m_Buttons || m_uiLastCharacter != input.m_uiLastCharacter;
  m_PrevButtons = m_Buttons;
  m_Buttons = input.m_Buttons;
  m_uiLastCharacter = input.m_uiLastCharacter;
  return bHasChanged;
}

EZ_ALWAYS_INLINE ezKeyState::Enum ezRmlUiInputProvider::GetButtonState(ezRmlUiInputButtons::Enum button) const
{
  if (m_Buttons.IsSet(button))
  {
    if (!m_PrevButtons.IsSet(button))
      return ezKeyState::Pressed;
    return ezKeyState::Down;
  }
  else
  {
    if (m_PrevButtons.IsSet(button))
      return ezKeyState::Released;
    return ezKeyState::Up;
  }
}
