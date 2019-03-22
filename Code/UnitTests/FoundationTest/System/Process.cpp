#include <FoundationTestPCH.h>

#include <Foundation/System/Process.h>

EZ_CREATE_SIMPLE_TEST_GROUP(System);

EZ_CREATE_SIMPLE_TEST(System, Process)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

  static const char* g_szTestMsg = "Tell me more!";

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Launch")
  {
    ezProcess proc;
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.m_Arguments.PushBack("timeout 1");

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::NotStarted);

    if (EZ_TEST_BOOL_MSG(proc.Launch().Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Finished);
    EZ_TEST_INT(proc.GetExitCode(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LaunchAsync / WaitToFinish")
  {
    ezProcess proc;
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.m_Arguments.PushBack("timeout 1"); // wait for 1 seconds

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::NotStarted);

    if (EZ_TEST_BOOL_MSG(proc.LaunchAsync().Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Running);
    EZ_TEST_BOOL(proc.WaitToFinish(ezTime::Seconds(5)).Succeeded());
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Finished);
    EZ_TEST_INT(proc.GetExitCode(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LaunchAsync / Terminate")
  {
    ezProcess proc;
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.m_Arguments.PushBack("timeout 10"); // wait for 10 seconds

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::NotStarted);

    if (EZ_TEST_BOOL_MSG(proc.LaunchAsync().Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Running);
    EZ_TEST_BOOL(proc.Terminate().Succeeded());
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Finished);
    EZ_TEST_INT(proc.GetExitCode(), -1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LaunchAsync / Detach")
  {
    ezTime tTerminate;

    {
      ezProcess proc;
      proc.m_sProcess = "C:/Windows/System32/cmd.exe";
      proc.m_Arguments.PushBack("/c");
      proc.m_Arguments.PushBack("timeout 10"); // wait for 10 seconds

      if (EZ_TEST_BOOL_MSG(proc.LaunchAsync().Succeeded(), "Failed to start process.").Failed())
        return;

      proc.Detach();

      tTerminate = ezTime::Now();
    }

    const ezTime tDiff = ezTime::Now() - tTerminate;
    EZ_TEST_BOOL_MSG(tDiff < ezTime::Seconds(1.0), "Destruction of ezProcess should be instant after Detach() was used.");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDOUT")
  {
    ezStringBuilder out;
    ezProcess proc;
    proc.m_onStdOut = [&out](ezStringView view) { out.Append(view); };
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.AddArgument(ezFmt("echo {}", g_szTestMsg));

    if (EZ_TEST_BOOL_MSG(proc.Launch().Succeeded(), "Failed to start process.").Failed())
      return;

    out.Trim("\r\n");
    EZ_TEST_STRING(out, g_szTestMsg);
    EZ_TEST_INT(proc.GetExitCode(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDERROR")
  {
    ezStringBuilder err;
    ezProcess proc;
    proc.m_onStdError = [&err](ezStringView view) { err.Append(view); };
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.m_Arguments.PushBack("NOT A VALID COMMAND");

    if (EZ_TEST_BOOL_MSG(proc.Launch().Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    EZ_TEST_INT(proc.GetExitCode(), 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDOUT_STDERROR")
  {
    ezStringBuilder out;
    ezStringBuilder err;
    ezProcess proc;
    proc.m_onStdOut = [&out](ezStringView view) { out.Append(view); };
    proc.m_onStdError = [&err](ezStringView view) { err.Append(view); };
    proc.m_sProcess = "C:/Windows/System32/cmd.exe";
    proc.m_Arguments.PushBack("/c");
    proc.AddArgument(ezFmt("echo {}", g_szTestMsg));

    if (EZ_TEST_BOOL_MSG(proc.Launch().Succeeded(), "Failed to start process.").Failed())
      return;

    out.Trim("\r\n");
    EZ_TEST_STRING(out, g_szTestMsg);
    EZ_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
    EZ_TEST_INT(proc.GetExitCode(), 0);
  }

#else
  ezLog::Warning("ezProcess functions are not implemented on this platform.");
#endif
}
