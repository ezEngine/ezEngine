#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>
#include <iostream>

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

ezInt32 ezConstructionCounterRelocatable::s_iConstructions = 0;
ezInt32 ezConstructionCounterRelocatable::s_iDestructions = 0;
ezInt32 ezConstructionCounterRelocatable::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounterRelocatable::s_iDestructionsLast = 0;

EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN("FoundationTest", "Foundation Tests")
{
  ezCommandLineUtils cmd;
  cmd.SetCommandLine(argc, (const char**)argv, ezCommandLineUtils::PreferOsArgs);

  // if the -cmd switch is set, FoundationTest.exe will execute a couple of simple operations and then close
  // this is used to test process launching (e.g. ezProcess)
  if (cmd.GetBoolOption("-cmd"))
  {
    // print something to stdout
    ezStringView sStdOut = cmd.GetStringOption("-stdout");
    if (!sStdOut.IsEmpty())
    {
      ezStringBuilder tmp;
      std::cout << sStdOut.GetData(tmp);
    }

    ezStringView sStdErr = cmd.GetStringOption("-stderr");
    if (!sStdErr.IsEmpty())
    {
      ezStringBuilder tmp;
      std::cerr << sStdErr.GetData(tmp);
    }

    // wait a little
    ezThreadUtils::Sleep(ezTime::Milliseconds(cmd.GetIntOption("-sleep")));

    // shutdown with exit code
    ezTestSetup::DeInitTestFramework(true);
    return cmd.GetIntOption("-exitcode");
  }
}
EZ_TESTFRAMEWORK_ENTRY_POINT_END()
