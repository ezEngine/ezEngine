#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Foundation/Configuration/CVar.h>

void ezQuakeConsole::ExecuteCommand(ezStringView input)
{
  const bool bBind = input.StartsWith_NoCase("bind ");
  const bool bUnbind = input.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    ezStringBuilder tmp;
    const char* szAfterCmd = ezStringUtils::FindWordEnd(input.GetData(tmp), ezStringUtils::IsWhiteSpace); // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = ezStringUtils::SkipCharacters(szAfterCmd, ezStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = ezStringUtils::FindWordEnd(szKeyNameStart, ezStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    ezStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey; // copy the word into a zero terminated string

    const char* szCommandToBind = ezStringUtils::SkipCharacters(szKeyNameEnd, ezStringUtils::IsWhiteSpace);

    if (bUnbind || ezStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  ezConsole::ExecuteCommand(input);
}

void ezQuakeConsole::BindKey(const char* szKey, const char* szCommand)
{
  ezStringBuilder s;
  s.Format("Binding key '{0}' to command '{1}'", szKey, szCommand);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys[szKey] = szCommand;
}

void ezQuakeConsole::UnbindKey(const char* szKey)
{
  ezStringBuilder s;
  s.Format("Unbinding key '{0}'", szKey);
  AddConsoleString(s, ezConsoleString::Type::Success);

  m_BoundKeys.Remove(szKey);
}

void ezQuakeConsole::ExecuteBoundKey(const char* szKey)
{
  auto it = m_BoundKeys.Find(szKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value().GetData());
  }
}



EZ_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
