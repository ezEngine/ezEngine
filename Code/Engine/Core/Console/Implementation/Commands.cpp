#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Foundation/Configuration/CVar.h>

void ezQuakeConsole::ExecuteCommand(ezStringView sInput)
{
  const bool bBind = sInput.StartsWith_NoCase("bind ");
  const bool bUnbind = sInput.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    ezStringBuilder tmp;
    const char* szAfterCmd = ezStringUtils::FindWordEnd(sInput.GetData(tmp), ezStringUtils::IsWhiteSpace);              // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = ezStringUtils::SkipCharacters(szAfterCmd, ezStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = ezStringUtils::FindWordEnd(szKeyNameStart, ezStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    ezStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey;                                                                                                         // copy the word into a zero terminated string

    const char* szCommandToBind = ezStringUtils::SkipCharacters(szKeyNameEnd, ezStringUtils::IsWhiteSpace);

    if (bUnbind || ezStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  ezConsole::ExecuteCommand(sInput);
}

void ezQuakeConsole::BindKey(ezStringView sKey, ezStringView sCommand)
{
  ezStringBuilder s;
  s.SetFormat("Binding key '{0}' to command '{1}'", sKey, sCommand);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys[sKey] = sCommand;
}

void ezQuakeConsole::UnbindKey(ezStringView sKey)
{
  ezStringBuilder s;
  s.SetFormat("Unbinding key '{0}'", sKey);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys.Remove(sKey);
}

void ezQuakeConsole::ExecuteBoundKey(ezStringView sKey)
{
  auto it = m_BoundKeys.Find(sKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value());
  }
}


