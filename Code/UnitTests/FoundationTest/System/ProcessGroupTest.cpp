#include <FoundationTestPCH.h>

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)

#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Utilities/CommandLineUtils.h>

EZ_CREATE_SIMPLE_TEST(System, ProcessGroup)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const ezStringBuilder pathToSelf = ezCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "WaitToFinish")
  {
    ezProcessGroup pgroup;
    ezStringBuilder out;

    ezMutex mutex;

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_onStdOut = [&out, &mutex](ezStringView view)
      {
        EZ_LOCK(mutex);
        out.Append(view);
      };

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("1000");
      opt.m_Arguments.PushBack("-stdout");
      opt.m_Arguments.PushBack("Na");

      EZ_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    // in a debugger with child debugging enabled etc. even 10 seconds can lead to timeouts due to long delays in the IDE
    EZ_TEST_BOOL(pgroup.WaitToFinish(ezTime::Seconds(60)).Succeeded());
    EZ_TEST_STRING(out, "NaNaNaNaNaNaNaNa"); // BATMAN!
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "TerminateAll")
  {
    ezProcessGroup pgroup;

    ezHybridArray<ezProcess, 8> procs;

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezProcessOptions opt;
      opt.m_sProcess = pathToSelf;

      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("60000");

      EZ_TEST_BOOL(pgroup.Launch(opt).Succeeded());
    }

    const ezTime tStart = ezTime::Now();
    EZ_TEST_BOOL(pgroup.TerminateAll().Succeeded());
    const ezTime tDiff = ezTime::Now() - tStart;

    EZ_TEST_BOOL(tDiff < ezTime::Seconds(10));
  }

#else
  ezLog::Warning("ezProcess functions are not implemented on this platform.");
#endif
}
#endif
