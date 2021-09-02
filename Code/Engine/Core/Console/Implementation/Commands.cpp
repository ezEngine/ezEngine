#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Configuration/CVar.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezConsoleFunctionBase);

void ezConsole::ProcessCommand(const char* szCmd)
{
  if (ezStringUtils::IsNullOrEmpty(szCmd))
    return;

  const bool bBind = ezStringUtils::StartsWith_NoCase(szCmd, "bind ");
  const bool bUnbind = ezStringUtils::StartsWith_NoCase(szCmd, "unbind ");

  if (bBind || bUnbind)
  {
    const char* szAfterCmd = ezStringUtils::FindWordEnd(szCmd, ezStringUtils::IsWhiteSpace); // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = ezStringUtils::SkipCharacters(szAfterCmd, ezStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = ezStringUtils::FindWordEnd(szKeyNameStart, ezStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    ezStringView sKey(szKeyNameStart, szKeyNameEnd);
    ezStringBuilder sKeyName = sKey; // copy the word into a zero terminated string

    const char* szCommandToBind = ezStringUtils::SkipCharacters(szKeyNameEnd, ezStringUtils::IsWhiteSpace);

    if (bUnbind || ezStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(sKeyName.GetData());
      return;
    }

    BindKey(sKeyName.GetData(), szCommandToBind);
    return;
  }

  if (m_CommandInterpreter)
  {
    ezCommandInterpreterState s;
    s.m_sInput = szCmd;
    m_CommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(szCmd);
  }
}

void ezConsole::BindKey(const char* szKey, const char* szCommand)
{
  ezStringBuilder s;
  s.Format("Binding key '{0}' to command '{1}'", szKey, szCommand);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys[szKey] = szCommand;
}

void ezConsole::UnbindKey(const char* szKey)
{
  ezStringBuilder s;
  s.Format("Unbinding key '{0}'", szKey);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys.Remove(szKey);
}

void ezConsole::ExecuteBoundKey(const char* szKey)
{
  auto it = m_BoundKeys.Find(szKey);

  if (it.IsValid())
    ProcessCommand(it.Value().GetData());
}



EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
