#include <FoundationPCH.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/Process_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/Process_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/Process_posix.h>
#else
#  error "Process functions are not implemented on current platform"
#endif

void ezProcess::AddArgument(const ezFormatString& arg)
{
  ezStringBuilder sb;
  m_Arguments.PushBack(arg.GetText(sb));
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

void ezProcess::BuildCommandLineString(const char* szProcess, ezStringBuilder& cmd) const
{
  ezStringBuilder proc = szProcess;

  // have to set the full path to the process as the very first argument
  cmd.Set("\"", proc, "\"");

  for (const auto& arg : m_Arguments)
  {
    if (arg.FindSubString(" ") != nullptr || arg.FindSubString("\t") != nullptr || arg.FindSubString("\n") != nullptr)
      cmd.Append(" \"", arg, "\"");
    else
      cmd.Append(" ", arg);
  }

  cmd.Trim(" ");
}
