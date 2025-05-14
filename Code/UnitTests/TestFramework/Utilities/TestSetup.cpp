#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/SystemInformation.h>

#ifdef EZ_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#else
#  include <TestFramework_Platform.h>
#endif

int ezTestSetup::s_iArgc = 0;
const char** ezTestSetup::s_pArgv = nullptr;

void OutputToConsole(ezTestOutput::Enum type, const char* szMsg);

ezTestFramework* ezTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int iArgc, const char** pArgv)
{
  s_iArgc = iArgc;
  s_pArgv = pArgv;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  RoInitialize(RO_INIT_MULTITHREADED);
#endif

  // without a proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(ezOSFile::GetUserDataFolder());
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append("ezEngine Tests/");
  sTestFolder.append(szTestName);

  std::string sTestDataSubFolder = "Data/UnitTests/";
  sTestDataSubFolder.append(szTestName);

#ifdef EZ_USE_QT
  ezTestFramework* pTestFramework = new ezQtTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#else
  ezTestFramework* pTestFramework = new ezTestFramework_Platform(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
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

  int argc = s_iArgc;
  char** argv = const_cast<char**>(s_pArgv);

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
    // Locale fixes required by various third party libraries like RmlGui.
    QLocale::setDefault(QLocale::C);
    const char* locales[] = {"C.UTF-8", "C.utf8", "UTF-8"};
    for (const char* szLocale : locales)
    {
      if (setlocale(LC_ALL, szLocale) != nullptr)
        break;
    }
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
#else
  // Run all the tests with the given order
  return pTestFramework->RunTests();
#endif
}

void ezTestSetup::DeInitTestFramework(bool bSilent /*= false*/)
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();

  ezStartup::ShutdownCoreSystems();

  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bKeepConsoleOpen && !bSilent)
  {
    if (ezSystemInformation::IsDebuggerAttached())
    {
      std::cout << "Press the any key to continue...\n";
      fflush(stdin);
      [[maybe_unused]] int c = getchar();
    }
  }

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
