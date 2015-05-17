#include <TestFramework/PCH.h>
#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>

#ifdef EZ_USE_QT
  #include <TestFramework/Framework/Qt/qtTestFramework.h>
  #include <TestFramework/Framework/Qt/qtTestGUI.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <conio.h>
#endif

int ezTestSetup::s_argc = 0;
const char** ezTestSetup::s_argv = nullptr;

ezTestFramework* ezTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int argc, const char** argv)
{
  s_argc = argc;
  s_argv = argv;

  // without at proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(BUILDSYSTEM_OUTPUT_FOLDER);
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append(szTestName);

#ifdef EZ_USE_QT
  ezTestFramework* pTestFramework = new ezQtTestFramework(szNiceTestName, sTestFolder.c_str(), argc, argv);
#else
  ezTestFramework* pTestFramework = new ezTestFramework(szNiceTestName, sTestFolder.c_str(), argc, argv);
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(ezOutputToHTML::OutputToHTML);

  return pTestFramework;
}

ezTestAppRun ezTestSetup::RunTests()
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();
#ifdef EZ_USE_QT
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bNoGUI)
  {
    return pTestFramework->RunTestExecutionLoop();
  }

  // Setup Qt Application
  
  int argc = s_argc;
  char** argv = const_cast<char**>(s_argv);
  QApplication app(argc, argv);

  app.setApplicationName(pTestFramework->GetTestName());
  
  ezQtTestGUI::SetDarkTheme();
  // Create main window
  ezQtTestGUI mainWindow(*static_cast<ezQtTestFramework*>(pTestFramework));
  mainWindow.show();

  app.exec();

  return ezTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTestExecutionLoop();
#endif
}

void ezTestSetup::DeInitTestFramework()
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezStringUtils::PrintStringLengthStatistics();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  TestSettings settings = pTestFramework->GetSettings();
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
  // This is needed as at least windows can't be bothered to write anything
  // to the output streams at all if it's not enough or the app is too fast.
  fflush(stdout);
  fflush(stderr);
  delete pTestFramework;
}

ezInt32 ezTestSetup::GetFailedTestCount()
{
  return ezTestFramework::GetInstance()->GetTestsFailedCount();
}


EZ_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestSetup);

