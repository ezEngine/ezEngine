#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezConsoleFunctionBase);

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

ezConsole::ezConsole() = default;

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

bool ezConsole::AutoComplete(ezStringBuilder& ref_sText)
{
  EZ_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void ezConsole::ExecuteCommand(ezStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  EZ_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void ezConsole::AddConsoleString(ezStringView sText, ezConsoleString::Type type /*= ezConsoleString::Type::Default*/)
{
  ezConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  ezConsoleEvent e;
  e.m_Type = ezConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void ezConsole::AddToInputHistory(ezStringView sText)
{
  EZ_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (ezInt32 i = 0; i < (ezInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (ezInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(ezMath::Min<ezUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (ezUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void ezConsole::RetrieveInputHistory(ezInt32 iHistoryUp, ezStringBuilder& ref_sResult)
{
  EZ_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = ezMath::Clamp<ezInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

ezResult ezConsole::SaveInputHistory(ezStringView sFile)
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezStringBuilder str;

  for (const ezString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    EZ_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return EZ_SUCCESS;
}

void ezConsole::LoadInputHistory(ezStringView sFile)
{
  ezFileReader file;
  if (file.Open(sFile).Failed())
    return;

  ezStringBuilder str;
  str.ReadAll(file);

  ezHybridArray<ezStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (ezUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}
