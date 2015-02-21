#include <CoreUtils/PCH.h>
#include <CoreUtils/Console/Console.h>
#include <CoreUtils/Console/LuaInterpreter.h>

ezConsole::ezConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

  SetCommandInterpreter(ezConsoleInterpreter::Lua);
}

ezConsole::~ezConsole()
{
  EnableLogOutput(false);
}

void ezConsole::AddConsoleString(const char* szText, const ezColor& color, bool bShowOnScreen)
{
  m_ConsoleStrings.PushFront();

  ConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = szText;
  cs.m_TextColor = color;
  cs.m_TimeStamp = ezTime::Now();
  cs.m_bShowOnScreen = bShowOnScreen;

  // Broadcast that we have added a string to the console
  {
    ConsoleEvent e;
    e.m_EventType = ConsoleEvent::StringAdded;
    e.m_AddedpConsoleString = &cs;

    m_Events.Broadcast(e);
  }

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);
}

void ezConsole::AddToInputHistory(const char* szString)
{
  m_iCurrentInputHistoryElement = -1;

  if (ezStringUtils::IsNullOrEmpty(szString))
    return;

  const ezString sString = szString;

  for (ezInt32 i = 0; i < (ezInt32) m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sString) // already in the History
    {
      // just move it to the front

      for (ezInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sString;
      return;
    }
  }

  m_InputHistory.SetCount(ezMath::Min<ezUInt32>(m_InputHistory.GetCount() + 1, 16));

  for (ezUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sString;
}

void ezConsole::SearchInputHistory(ezInt32 iHistoryUp)
{
  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = ezMath::Clamp<ezInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
    m_sInputLine = m_InputHistory[m_iCurrentInputHistoryElement];

  m_iCaretPosition = m_sInputLine.GetCharacterCount();
}



void ezConsole::LogHandler(const ezLoggingEventData& data)
{
  bool bShow = false;
  ezColor color = ezColor::White;

  switch (data.m_EventType)
  {
  case ezLogMsgType::BeginGroup:
  case ezLogMsgType::EndGroup:
  case ezLogMsgType::None:
  case ezLogMsgType::ENUM_COUNT:
  case ezLogMsgType::All:
    return;

  case ezLogMsgType::ErrorMsg:
    color = ezColor(1.0f, 0.1f, 0.1f);
    bShow = true;
    break;
  case ezLogMsgType::SeriousWarningMsg:
    color = ezColor(1.0f, 0.4f, 0.1f);
    bShow = true;
    break;
  case ezLogMsgType::WarningMsg:
    color = ezColor(1.0f, 0.6f, 0.1f);
    break;
  case ezLogMsgType::SuccessMsg:
    color = ezColor(0.1f, 1.0f, 0.1f);
    break;
  case ezLogMsgType::InfoMsg:
    break;
  case ezLogMsgType::DevMsg:
    color = ezColor(200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f);
    break;
  case ezLogMsgType::DebugMsg:
    color = ezColor(180.0f / 255.0f, 0.1f, 1.0f);
    break;
  }

  ezStringBuilder sFormat;
  sFormat.Format("%*s%s", data.m_uiIndentation, "", data.m_szText);

  AddConsoleString(sFormat.GetData(), color, bShow);
}

void ezConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezConsole::LogHandler, this));
  }
  else
  {
    ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezConsole::LogHandler, this));
  }
}

void ezConsole::SaveState(ezStreamWriterBase& Stream) const
{
  ezUInt8 uiVersion = 1;
  Stream << uiVersion;

  Stream << m_InputHistory.GetCount();
  for (ezUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    Stream << m_InputHistory[i];
  }

  Stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    Stream << it.Key();
    Stream << it.Value();
  }
}

void ezConsole::LoadState(ezStreamReaderBase& Stream)
{
  ezUInt8 uiVersion = 0;
  Stream >> uiVersion;

  if (uiVersion == 1)
  {
    ezUInt32 count = 0;
    Stream >> count;
    m_InputHistory.SetCount(count);

    for (ezUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      Stream >> m_InputHistory[i];
    }

    Stream >> count;

    ezString sKey;
    ezString sValue;

    for (ezUInt32 i = 0; i < count; ++i)
    {
      Stream >> sKey;
      Stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}




EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Console_Implementation_Console);

