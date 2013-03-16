#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>
#include <TestFramework/Utilities/TestOrder.h>
#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>

#if EZ_PLATFORM_WINDOWS
  #include <conio.h>
#endif

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

int main()
{
  // without at proper file system the current working directory is pretty much useless
  std::string sOutputFolder = BUILDSYSTEM_OUTPUT_FOLDER;
  
  std::string sTestSettings = sOutputFolder; 
  sTestSettings += std::string("/TestSettings.txt");

  std::string sHTMLOutput = sOutputFolder; 
  sHTMLOutput += std::string("/FoundationTests.htm");

  TestSettings settings;

  // Scope to make sure all file handles are closed before the result is opened externally
  {
    ezOutputToHTML HTMLWriter(sHTMLOutput.c_str(), "Foundation Tests");

    // Register some output handlers to forward all the messages to the console and to an HTML file
    ezTestFramework::RegisterOutputHandler(OutputToConsole);
    ezTestFramework::RegisterOutputHandler(ezOutputToHTML::OutputToHTML);

    // array of all the tests
    std::deque<ezTestEntry> AllTests;

    // figure out which tests exist
    ezTestFramework::GatherAllTests(AllTests);

    // sort the tests alphabetically for now
    SortTestsAlphabetically(AllTests);

    // load the test order from file, if that file does not exist, the array is not modified
    LoadTestOrder(sTestSettings.c_str(), AllTests, settings);

    // save the current order back to the same file
    SaveTestOrder(sTestSettings.c_str(), AllTests, settings);

    ezTestFramework::SetAssertOnTestFail(settings.m_bAssertOnTestFail);

    // run all the tests with the given order
    ezTestFramework::ExecuteAllTests(AllTests);

    if (ezTestFramework::GetTestsFailedCount() == 0)
      ezTestFramework::Output(ezTestOutput::FinalResult, "All tests passed.");
    else
      ezTestFramework::Output(ezTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", ezTestFramework::GetTestsFailedCount(), ezTestFramework::GetTestsPassedCount());
  }

  ezLog::AddLogWriter(ezLog_ConsoleWriter::LogMessageHandler);
  ezStringUtils::PrintStringLengthStatistics();

  #if EZ_PLATFORM_WINDOWS
    if (settings.m_bOpenHtmlOutput)
    {
      // opens the html file in a browser
      ShellExecuteA(NULL, "open", sHTMLOutput.c_str(), NULL, NULL, SW_SHOW);
    }

    if (settings.m_bKeepConsoleOpen)
    {
      if (IsDebuggerPresent())
      {
        std::cout << "Press the any key to continue...\n";
        fflush(stdin);
        int iRet = _getch();
        EZ_ANALYSIS_ASSUME(iRet < 256);
      }
    }
  #endif

  return ezTestFramework::GetTestsFailedCount();
}