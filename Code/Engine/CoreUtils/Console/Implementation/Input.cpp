#include <CoreUtils/PCH.h>
#include <CoreUtils/Console/Console.h>
#include <Core/Input/InputManager.h>

bool ezConsole::ProcessInputCharacter(ezUInt32 uiChar)
{
  switch (uiChar)
  {
  case 27: // Escape
    ClearInputLine();
    return false;

  case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
    }
    return false;

  case '\t':
    AutoCompleteInputLine();
    return false;

  case 13: // Enter
    AddToInputHistory(m_sInputLine.GetData());
    ProcessCommand(m_sInputLine.GetData());
    ClearInputLine();
    return false;
  }

  return true;
}

bool ezConsole::FilterInputCharacter(ezUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void ezConsole::ClampCaretPosition()
{
  m_iCaretPosition = ezMath::Clamp<ezInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void ezConsole::MoveCaret(ezInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void ezConsole::Scroll(ezInt32 iLines)
{
  m_iScrollPosition = ezMath::Clamp<ezInt32>(m_iScrollPosition + iLines, 0, ezMath::Max<ezInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void ezConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition = 0;
  m_iScrollPosition = 0;
  m_iCurrentInputHistoryElement = -1;
}

void ezConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_iScrollPosition = 0; 
}

void ezConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void ezConsole::RemoveCharacter(ezUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());
}

void ezConsole::AddInputCharacter(ezUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  ezUInt32 uiString[2] = { uiChar, 0 };

  m_sInputLine.Insert(it.GetData(), ezStringUtf8(uiString).GetData());

  MoveCaret(1);
}

void ezConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    ezInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyLeft;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyRight;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyHome;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyEnd;
    ezInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDelete;
    ezInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyPageUp;
    ezInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyPageDown;
    ezInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyUp;
    ezInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyDown;
    ezInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF2;
    ezInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = ezInputSlot_KeyF3;
    ezInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (ezInputManager::GetInputActionState("Console", "MoveCaretLeft") == ezKeyState::Pressed)
      MoveCaret(-1);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretRight") == ezKeyState::Pressed)
      MoveCaret(1);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretStart") == ezKeyState::Pressed)
      MoveCaret(-1000);
    if (ezInputManager::GetInputActionState("Console", "MoveCaretEnd") == ezKeyState::Pressed)
      MoveCaret(1000);
    if (ezInputManager::GetInputActionState("Console", "DeleteCharacter") == ezKeyState::Pressed)
      DeleteNextCharacter();
    if (ezInputManager::GetInputActionState("Console", "ScrollUp") == ezKeyState::Pressed)
      Scroll(10);
    if (ezInputManager::GetInputActionState("Console", "ScrollDown") == ezKeyState::Pressed)
      Scroll(-10);
    if (ezInputManager::GetInputActionState("Console", "HistoryUp") == ezKeyState::Pressed)
      SearchInputHistory(1);
    if (ezInputManager::GetInputActionState("Console", "HistoryDown") == ezKeyState::Pressed)
      SearchInputHistory(-1);

    const ezUInt32 uiChar = ezInputManager::RetrieveLastCharacter();

    if (uiChar != '\0')
      AddInputCharacter(uiChar);
  }
  else
  {
    const ezUInt32 uiChar = ezInputManager::RetrieveLastCharacter(false);

    char szCmd[16] = "";
    char* szIterator = szCmd;
    ezUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ProcessCommand(GetInputHistory()[0].GetData());
  }

  if (ezInputManager::GetInputActionState("Console", "RepeatSecondLast") == ezKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ProcessCommand(GetInputHistory()[1].GetData());
  }

}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Console_Implementation_Input);

