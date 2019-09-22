#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
// Include inline file
#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    include <Foundation/System/Implementation/Win/Process_win.h>
#  elif EZ_ENABLED(EZ_PLATFORM_OSX)
#    include <Foundation/System/Implementation/OSX/Process_OSX.h>
#  elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#    include <Foundation/System/Implementation/Posix/Process_posix.h>
#  else
#    error "Process functions are not implemented on current platform"
#  endif

#  include <Strings/Implementation/StringIterator.h>

ezProcess::ezProcess(ezProcess&& rhs) = default;

void ezProcessOptions::AddArgument(const ezFormatString& arg)
{
  ezStringBuilder sb;
  arg.GetText(sb);
  sb.Trim(" \t\n");

  m_Arguments.PushBack(sb);
}

void ezProcessOptions::AddCommandLine(const char* szCmdLine)
{
  ezStringBuilder curArg;

  ezStringView cmdView(szCmdLine);

  bool isInString = false;

  for (auto it = cmdView.GetIteratorFront(); it.IsValid(); ++it)
  {
    bool commit = false;
    bool commitEmpty = false;
    bool append = true;

    if (it.GetCharacter() == '\"')
    {
      append = false;

      if (isInString)
      {
        commitEmpty = true; // push-back even empty strings (they are there for a purpose)
      }
      else
      {
        commit = true; // only commit non-empty stuff that is not a string argument
      }

      isInString = !isInString;
    }
    else if (it.GetCharacter() == ' ')
    {
      if (!isInString)
      {
        commit = true;
        append = false;
      }
    }

    if (commitEmpty || (commit && !curArg.IsEmpty()))
    {
      m_Arguments.PushBack(curArg);
      curArg.Clear();
    }

    if (append)
    {
      curArg.Append(it.GetCharacter());
    }
  }

  if (!curArg.IsEmpty())
  {
    m_Arguments.PushBack(curArg);
    curArg.Clear();
  }
}

ezInt32 ezProcess::GetExitCode() const
{
  if (m_iExitCode == -0xFFFF)
  {
    // this may update m_iExitCode, if the state has switched to 'finished'
    GetState();
  }

  return m_iExitCode;
}

void ezProcessOptions::BuildCommandLineString(ezStringBuilder& cmd) const
{
  for (const auto& arg0 : m_Arguments)
  {
    ezStringView arg = arg0;

    while (arg.StartsWith("\""))
      arg.Shrink(1, 0);

    while (arg.EndsWith("\""))
      arg.Shrink(0, 1);

    // also wrap empty arguments in quotes, otherwise they would get lost
    if (arg.IsEmpty() || arg.FindSubString(" ") != nullptr || arg.FindSubString("\t") != nullptr || arg.FindSubString("\n") != nullptr)
    {
      cmd.Append(" \"");
      cmd.Append(arg);
      cmd.Append("\"");
    }
    else
    {
      cmd.Append(" ");
      cmd.Append(arg);
    }
  }

  cmd.Trim(" ");
}

void ezProcess::BuildFullCommandLineString(const ezProcessOptions& opt, const char* szProcess, ezStringBuilder& cmd) const
{
  // have to set the full path to the process as the very first argument
  cmd.Set("\"", szProcess, "\"");

  opt.BuildCommandLineString(cmd);
}
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_System_Implementation_Process);
