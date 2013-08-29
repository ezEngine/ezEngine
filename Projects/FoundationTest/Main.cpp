#include <PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <TestFramework/Utilities/TestOrder.h>
#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>
#ifdef EZ_USE_QT
  #include <TestFramework/Framework/Qt/qtTestFramework.h>
  #include <TestFramework/Framework/Qt/qtTestGUI.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <conio.h>
#endif

ezInt32 ezConstructionCounter::s_iConstructions = 0;
ezInt32 ezConstructionCounter::s_iDestructions = 0;
ezInt32 ezConstructionCounter::s_iConstructionsLast = 0;
ezInt32 ezConstructionCounter::s_iDestructionsLast = 0;

int main(int argc, char **argv)
{
  // without at proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(BUILDSYSTEM_OUTPUT_FOLDER) + std::string("/FoundationTest");

#ifdef EZ_USE_QT
  ezQtTestFramework testFramework("Foundation Tests", sTestFolder.c_str());
#else
  ezTestFramework testFramework("Foundation Tests", sTestFolder.c_str());
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  testFramework.RegisterOutputHandler(OutputToConsole);
  testFramework.RegisterOutputHandler(ezOutputToHTML::OutputToHTML);

#ifdef EZ_USE_QT
  ezQtTestGUI::SetDarkTheme();
  QApplication app(argc, argv);
  app.setApplicationName("ezFoundationTest");
  
  ezQtTestGUI mainWindow(testFramework);
  mainWindow.show();
  app.exec();
#else
  // run all the tests with the given order
  testFramework.ExecuteAllTests();
#endif

  ezLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezStringUtils::PrintStringLengthStatistics();

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  TestSettings settings = testFramework.GetSettings();
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

  return testFramework.GetTestsFailedCount();
}