#include <CoreUtils/PCH.h>
#include <CoreUtils/Console/Console.h>

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
  // if you want to support full unicode characters in the console, override this function and change this restriction
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

