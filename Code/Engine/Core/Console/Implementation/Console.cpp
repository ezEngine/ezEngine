#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>

ezConsole::ezConsole()
{
  // BEGIN-DOCS-CODE-SNIPPET: console-demo1
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);
  // END-DOCS-CODE-SNIPPET

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(EZ_DEFAULT_NEW(ezCommandInterpreterLua));
#endif
}

ezConsole::~ezConsole()
{
  EnableLogOutput(false);
}

void ezConsole::AddConsoleString(const char* szText, const ezColor& color, bool bShowOnScreen)
{
  EZ_LOCK(m_Mutex);

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

const ezDeque<ezConsole::ezConsole::ConsoleString>& ezConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void ezConsole::AddToInputHistory(const char* szString)
{
  EZ_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (ezStringUtils::IsNullOrEmpty(szString))
    return;

  const ezString sString = szString;

  for (ezInt32 i = 0; i < (ezInt32)m_InputHistory.GetCount(); i++)
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
  EZ_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = ezMath::Clamp<ezInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
    m_sInputLine = m_InputHistory[m_iCurrentInputHistoryElement];

  m_iCaretPosition = m_sInputLine.GetCharacterCount();
}


void ezConsole::ReplaceInput(const char* sz)
{
  m_sInputLine = sz;
  ClampCaretPosition();
}

void ezConsole::LogHandler(const ezLoggingEventData& data)
{
  bool bShow = false;
  ezColor color = ezColor::White;

  switch (data.m_EventType)
  {
    case ezLogMsgType::GlobalDefault:
    case ezLogMsgType::Flush:
    case ezLogMsgType::BeginGroup:
    case ezLogMsgType::EndGroup:
    case ezLogMsgType::None:
    case ezLogMsgType::ENUM_COUNT:
    case ezLogMsgType::All:
      return;

    case ezLogMsgType::ErrorMsg:
      color = ezColor(1.0f, 0.2f, 0.2f);
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
      color = ezColor(0.6f, 0.6f, 0.6f);
      break;
    case ezLogMsgType::DebugMsg:
      color = ezColor(0.4f, 0.6f, 0.8f);
      break;
  }

  ezStringBuilder sFormat;
  sFormat.Printf("%*s%s", data.m_uiIndentation, "", data.m_szText);

  AddConsoleString(sFormat.GetData(), color, bShow);
}

void ezConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    ezStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
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

void ezConsole::SaveState(ezStreamWriter& Stream) const
{
  EZ_LOCK(m_Mutex);

  const ezUInt8 uiVersion = 1;
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

void ezConsole::LoadState(ezStreamReader& Stream)
{
  EZ_LOCK(m_Mutex);

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

void ezCommandInterpreterState::AddOutputLine(const ezFormatString& text, ezCommandOutputLine::Type type /*= ezCommandOutputLine::Type::Default*/)
{
  auto& line = m_sOutput.ExpandAndGetRef();
  line.m_Type = type;

  ezStringBuilder tmp;
  line.m_sText = text.GetText(tmp);
}

EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Console);
