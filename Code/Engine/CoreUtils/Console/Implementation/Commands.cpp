#include <CoreUtils/PCH.h>
#include <CoreUtils/Console/Console.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Logging/Log.h>
#include <CoreUtils/Console/ConsoleFunction.h>

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

    const char* szKeyNameStart = ezStringUtils::SkipCharacters(szAfterCmd, ezStringUtils::IsWhiteSpace); // go to the next word
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

  // Broadcast that we are about to process some command
  {
    ConsoleEvent e;
    e.m_EventType = ConsoleEvent::BeforeProcessCommand;
    e.m_szCommand = szCmd;

    m_Events.Broadcast(e);
  }

  ezResult res = EZ_FAILURE;

  if (m_CommandProcessor.IsValid())
    res = m_CommandProcessor(szCmd, this);
  else
    AddConsoleString(szCmd, ezColor::White, true);

  // Broadcast that we have processed a command
  {
    ConsoleEvent e;
    e.m_EventType = res.Succeeded() ? ConsoleEvent::ProcessCommandSuccess : ConsoleEvent::ProcessCommandFailure;
    e.m_szCommand = szCmd;

    m_Events.Broadcast(e);
  }
}

void ezConsole::BindKey(const char* szKey, const char* szCommand)
{
  ezStringBuilder s;
  s.Format("Binding key '%s' to command '%s'", szKey, szCommand);
  AddConsoleString(s.GetData(), ezColor(50 / 255.0f, 1, 50 / 255.0f));

  m_BoundKeys[szKey] = szCommand;
}

void ezConsole::UnbindKey(const char* szKey)
{
  ezStringBuilder s;
  s.Format("Unbinding key '%s'", szKey);
  AddConsoleString(s.GetData(), ezColor(50 / 255.0f, 1, 50 / 255.0f));

  m_BoundKeys.Remove(szKey);
}

void ezConsole::ExecuteBoundKey(const char* szKey)
{
  auto it = m_BoundKeys.Find(szKey);

  if (it.IsValid())
    ProcessCommand(it.Value().GetData());
}



EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Console_Implementation_Commands);

