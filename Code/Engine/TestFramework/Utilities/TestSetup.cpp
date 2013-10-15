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

ezTestFramework* ezTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName)
{
  // without at proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(BUILDSYSTEM_OUTPUT_FOLDER);
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append(szTestName);

#ifdef EZ_USE_QT
  ezTestFramework* pTestFramework = new ezQtTestFramework(szNiceTestName, sTestFolder.c_str());
#else
  ezTestFramework* pTestFramework = new ezTestFramework(szNiceTestName, sTestFolder.c_str());
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(ezOutputToHTML::OutputToHTML);

  return pTestFramework;
}

ezInt32 ezTestSetup::RunTests(int argc, char **argv)
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();
#ifdef EZ_USE_QT
  // Setup Qt Application
  ezQtTestGUI::SetDarkTheme();
  QApplication app(argc, argv);
  app.setApplicationName(pTestFramework->GetTestName());
  
  // Create main window
  ezQtTestGUI mainWindow(*static_cast<ezQtTestFramework*>(pTestFramework));
  mainWindow.show();
  app.exec();
#else
  // Run all the tests with the given order
  pTestFramework->ExecuteAllTests();
#endif
  return pTestFramework->GetTestsFailedCount();
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
  delete pTestFramework;
}
