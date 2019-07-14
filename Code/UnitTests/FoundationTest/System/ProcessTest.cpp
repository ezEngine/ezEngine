#include <FoundationTestPCH.h>

#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(System);

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)

EZ_CREATE_SIMPLE_TEST(System, Process)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Command Line")
  {
    ezProcessOptions proc;
    proc.m_Arguments.PushBack("-bla");
    proc.m_Arguments.PushBack("blub blub");
    proc.m_Arguments.PushBack("\"di dub\"");
    proc.AddArgument(" -test ");
    proc.AddArgument("-hmpf {}", 27);
    proc.AddCommandLine("-a b   -c  d  -e \"f g h\" ");

    ezStringBuilder cmdLine;
    proc.BuildCommandLineString(cmdLine);

    EZ_TEST_STRING(cmdLine, "-bla \"blub blub\" \"di dub\" -test \"-hmpf 27\" -a b -c d -e \"f g h\"");
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

  static const char* g_szTestMsg = "Tell me more!";

  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const ezStringBuilder pathToSelf = ezCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Execute")
  {
    ezProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    ezInt32 exitCode = -1;

    if (EZ_TEST_BOOL_MSG(ezProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_INT(exitCode, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Launch / WaitToFinish")
  {
    ezProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    ezProcess proc;
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::NotStarted);

    if (EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Running);
    EZ_TEST_BOOL(proc.WaitToFinish(ezTime::Seconds(5)).Succeeded());
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Finished);
    EZ_TEST_INT(proc.GetExitCode(), 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Launch / Terminate")
  {
    ezProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("10000");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("0");

    ezProcess proc;
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::NotStarted);

    if (EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Running);
    EZ_TEST_BOOL(proc.Terminate().Succeeded());
    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Finished);
    EZ_TEST_INT(proc.GetExitCode(), -1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Launch / Detach")
  {
    ezTime tTerminate;

    {
      ezProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("10000");

      ezProcess proc;
      if (EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process.").Failed())
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
    ezProcessOptions opt;
    opt.m_onStdOut = [&out](ezStringView view) { out.Append(view); };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (EZ_TEST_BOOL_MSG(ezProcess::Execute(opt).Succeeded(), "Failed to start process.").Failed())
      return;

    out.Trim("\r\n");
    EZ_TEST_STRING(out, g_szTestMsg);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDERROR")
  {
    ezStringBuilder err;
    ezProcessOptions opt;
    opt.m_onStdError = [&err](ezStringView view) { err.Append(view); };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stderr");
    opt.m_Arguments.PushBack("NOT A VALID COMMAND");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("1");

    ezInt32 exitCode = 0;

    if (EZ_TEST_BOOL_MSG(ezProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process.").Failed())
      return;

    EZ_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    EZ_TEST_INT(exitCode, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDOUT_STDERROR")
  {
    ezStringBuilder out;
    ezStringBuilder err;
    ezProcessOptions opt;
    opt.m_onStdOut = [&out](ezStringView view) { out.Append(view); };
    opt.m_onStdError = [&err](ezStringView view) { err.Append(view); };
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (EZ_TEST_BOOL_MSG(ezProcess::Execute(opt).Succeeded(), "Failed to start process.").Failed())
      return;

    out.Trim("\r\n");
    EZ_TEST_STRING(out, g_szTestMsg);
    EZ_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
  }

#else
  ezLog::Warning("ezProcess functions are not implemented on this platform.");
#endif
}
#endif
