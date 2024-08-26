#include <FoundationTest/FoundationTestPCH.h>

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

  static const char* g_szTestMsg = "Tell me more!\nAnother line\n520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\nThat's all";
  static const char* g_szTestMsgLine0 = "Tell me more!\n";
  static const char* g_szTestMsgLine1 = "Another line\n";
  static const char* g_szTestMsgLine2 = "520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\n";
  static const char* g_szTestMsgLine3 = "That's all";


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

    if (!EZ_TEST_BOOL_MSG(ezProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
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

    if (!EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    EZ_TEST_BOOL(proc.GetState() == ezProcessState::Running);
    EZ_TEST_BOOL(proc.WaitToFinish(ezTime::MakeFromSeconds(5)).Succeeded());
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

    if (!EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
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
      if (!EZ_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
        return;

      proc.Detach();

      tTerminate = ezTime::Now();
    }

    const ezTime tDiff = ezTime::Now() - tTerminate;
    EZ_TEST_BOOL_MSG(tDiff < ezTime::MakeFromSeconds(1.0), "Destruction of ezProcess should be instant after Detach() was used.");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDOUT")
  {
    ezDynamicArray<ezStringBuilder> lines;
    ezStringBuilder out;
    ezProcessOptions opt;
    opt.m_onStdOut = [&](ezStringView sView)
    {
      out.Append(sView);
      lines.PushBack(sView);
    };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!EZ_TEST_BOOL_MSG(ezProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (EZ_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    EZ_TEST_STRING(out, g_szTestMsg);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDERROR")
  {
    ezStringBuilder err;
    ezProcessOptions opt;
    opt.m_onStdError = [&err](ezStringView sView)
    { err.Append(sView); };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stderr");
    opt.m_Arguments.PushBack("NOT A VALID COMMAND");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("1");

    ezInt32 exitCode = 0;

    if (!EZ_TEST_BOOL_MSG(ezProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    EZ_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    EZ_TEST_INT(exitCode, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "STDOUT_STDERROR")
  {
    ezDynamicArray<ezStringBuilder> lines;
    ezStringBuilder out;
    ezStringBuilder err;
    ezProcessOptions opt;
    opt.m_onStdOut = [&](ezStringView sView)
    {
      out.Append(sView);
      lines.PushBack(sView);
    };
    opt.m_onStdError = [&err](ezStringView sView)
    { err.Append(sView); };
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!EZ_TEST_BOOL_MSG(ezProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (EZ_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      EZ_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    EZ_TEST_STRING(out, g_szTestMsg);
    EZ_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
  }
}
#endif
