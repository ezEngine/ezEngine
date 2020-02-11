#include <TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/System/CrashHandler.h>

#ifdef EZ_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <TestFramework/Framework/Uwp/uwpTestFramework.h>
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

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
  {
    std::cout << "Failed to init WinRT." << std::endl;
  }
#endif

  // without at proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(ezOSFile::GetUserDataFolder());
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append("ezEngine Tests/");
  sTestFolder.append(szTestName);

  std::string sTestDataSubFolder = "Data/UnitTests/";
  sTestDataSubFolder.append(szTestName);

#ifdef EZ_USE_QT
  ezTestFramework* pTestFramework = new ezQtTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), argc, argv);
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Command line args in UWP are handled differently and can't be retrieved from the main function.
  ezTestFramework* pTestFramework = new ezUwpTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), 0, nullptr);
#else
  ezTestFramework* pTestFramework = new ezTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), argc, argv);
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(ezOutputToHTML::OutputToHTML);

  ezCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(pTestFramework->GetAbsOutputPath(), szTestName);
  ezCrashHandler::SetCrashHandler(&ezCrashHandler_WriteMiniDump::g_Instance);

  return pTestFramework;
}

ezTestAppRun ezTestSetup::RunTests()
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();

  // Todo: Incorporate all the below in a virtual call of testFramework?
#ifdef EZ_USE_QT
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bNoGUI)
  {
    return pTestFramework->RunTestExecutionLoop();
  }

  // Setup Qt Application

  int argc = s_argc;
  char** argv = const_cast<char**>(s_argv);

  if (qApp != nullptr)
  {
    bool ok = false;
    int iCount = qApp->property("Shared").toInt(&ok);
    EZ_ASSERT_DEV(ok, "Existing QApplication was not constructed by EZ!");
    qApp->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    new QApplication(argc, argv);
    qApp->setProperty("Shared", QVariant::fromValue((int)1));
    qApp->setApplicationName(pTestFramework->GetTestName());
    ezQtTestGUI::SetDarkTheme();
  }
  
  // Create main window
  {
    ezQtTestGUI mainWindow(*static_cast<ezQtTestFramework*>(pTestFramework));
    mainWindow.show();

    qApp->exec();
  }
  {
    const int iCount = qApp->property("Shared").toInt();
    if (iCount == 1)
    {
      delete qApp;
    }
    else
    {
      qApp->setProperty("Shared", QVariant::fromValue(iCount - 1));
    }
  }

  return ezTestAppRun::Quit;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  static_cast<ezUwpTestFramework*>(pTestFramework)->Run();
  return ezTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTestExecutionLoop();
#endif
}

void ezTestSetup::DeInitTestFramework(bool bSilent /*= false*/)
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();

  ezStartup::ShutdownCoreSystems();

  // In the UWP case we never initialized this thread for ez, so we can't do log output now.
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  if (!bSilent)
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezStringUtils::PrintStringLengthStatistics();
  }
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bKeepConsoleOpen && !bSilent)
  {
    if (IsDebuggerPresent())
    {
      std::cout << "Press the any key to continue...\n";
      fflush(stdin);
      int iRet = _getch();
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
