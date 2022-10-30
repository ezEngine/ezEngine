#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezConsoleFunctionBase);

ezQuakeConsole::ezQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(EZ_DEFAULT_NEW(ezCommandInterpreterLua));
#endif
}

ezQuakeConsole::~ezQuakeConsole()
{
  EnableLogOutput(false);
}

void ezQuakeConsole::AddConsoleString(ezStringView text, ezConsoleString::Type type)
{
  EZ_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  ezConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = text;
  cs.m_Type = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);

  ezConsole::AddConsoleString(text, type);
}

const ezDeque<ezConsoleString>& ezQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void ezQuakeConsole::LogHandler(const ezLoggingEventData& data)
{
  ezConsoleString::Type type = ezConsoleString::Type::Default;

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
      type = ezConsoleString::Type::Error;
      break;

    case ezLogMsgType::SeriousWarningMsg:
      type = ezConsoleString::Type::SeriousWarning;
      break;

    case ezLogMsgType::WarningMsg:
      type = ezConsoleString::Type::Warning;
      break;

    case ezLogMsgType::SuccessMsg:
      type = ezConsoleString::Type::Success;
      break;

    case ezLogMsgType::InfoMsg:
      break;

    case ezLogMsgType::DevMsg:
      type = ezConsoleString::Type::Dev;
      break;

    case ezLogMsgType::DebugMsg:
      type = ezConsoleString::Type::Debug;
      break;
  }

  ezStringBuilder sFormat;
  sFormat.Printf("%*s%s", data.m_uiIndentation, "", data.m_szText);

  AddConsoleString(sFormat.GetData(), type);
}

void ezQuakeConsole::InputStringChanged()
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

void ezQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezQuakeConsole::LogHandler, this));
  }
  else
  {
    ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezQuakeConsole::LogHandler, this));
  }
}

void ezQuakeConsole::SaveState(ezStreamWriter& Stream) const
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

void ezQuakeConsole::LoadState(ezStreamReader& Stream)
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

void ezCommandInterpreterState::AddOutputLine(const ezFormatString& text, ezConsoleString::Type type /*= ezCommandOutputLine::Type::Default*/)
{
  auto& line = m_sOutput.ExpandAndGetRef();
  line.m_Type = type;

  ezStringBuilder tmp;
  line.m_sText = text.GetText(tmp);
}

ezColor ezConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case ezConsoleString::Type::Default:
      return ezColor::White;

    case ezConsoleString::Type::Error:
      return ezColor(1.0f, 0.2f, 0.2f);

    case ezConsoleString::Type::SeriousWarning:
      return ezColor(1.0f, 0.4f, 0.1f);

    case ezConsoleString::Type::Warning:
      return ezColor(1.0f, 0.6f, 0.1f);

    case ezConsoleString::Type::Note:
      return ezColor(1, 200.0f / 255.0f, 0);

    case ezConsoleString::Type::Success:
      return ezColor(0.1f, 1.0f, 0.1f);

    case ezConsoleString::Type::Executed:
      return ezColor(1.0f, 0.5f, 0.0f);

    case ezConsoleString::Type::VarName:
      return ezColorGammaUB(255, 210, 0);

    case ezConsoleString::Type::FuncName:
      return ezColorGammaUB(100, 255, 100);

    case ezConsoleString::Type::Dev:
      return ezColor(0.6f, 0.6f, 0.6f);

    case ezConsoleString::Type::Debug:
      return ezColor(0.4f, 0.6f, 0.8f);

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezColor::White;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

ezConsole::ezConsole()
{
}

ezConsole::~ezConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void ezConsole::SetMainConsole(ezConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

ezConsole* ezConsole::GetMainConsole()
{
  return s_pMainConsole;
}

ezConsole* ezConsole::s_pMainConsole = nullptr;

bool ezConsole::AutoComplete(ezStringBuilder& text)
{
  EZ_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = text;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (text != s.m_sInput)
    {
      text = s.m_sInput;
      return true;
    }
  }

  return false;
}

void ezConsole::ExecuteCommand(ezStringView input)
{
  if (input.IsEmpty())
    return;

  EZ_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = input;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(input);
  }
}

void ezConsole::AddConsoleString(ezStringView text, ezConsoleString::Type type /*= ezConsoleString::Type::Default*/)
{
  ezConsoleString cs;
  cs.m_sText = text;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  ezConsoleEvent e;
  e.m_Type = ezConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void ezConsole::AddToInputHistory(ezStringView text)
{
  EZ_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (text.IsEmpty())
    return;

  for (ezInt32 i = 0; i < (ezInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == text) // already in the History
    {
      // just move it to the front

      for (ezInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = text;
      return;
    }
  }

  m_InputHistory.SetCount(ezMath::Min<ezUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (ezUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = text;
}

void ezConsole::RetrieveInputHistory(ezInt32 iHistoryUp, ezStringBuilder& result)
{
  EZ_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = ezMath::Clamp<ezInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    result = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Console);
