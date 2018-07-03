#include <PCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/Utilities/StackTracer.h>

#ifdef EZ_USE_QT
#include <TestFramework/Framework/Qt/qtTestFramework.h>
#include <TestFramework/Framework/Qt/qtTestGUI.h>
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <TestFramework/Framework/Uwp/uwpTestFramework.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <conio.h>
#else if EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#include <csignal>
#include <cxxabi.h>
#endif

namespace ExceptionHandler
{
  static void PrintHelper(const char* szText)
  {
    printf("%s", szText);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    OutputDebugStringW(ezStringWChar(szText).GetData());
#endif
    fflush(stdout);
    fflush(stderr);
  };

  static void Print(const char* szFormat, ...)
  {
    char buff[1024];
    va_list args;
    va_start(args, szFormat);
    vsprintf(buff, szFormat, args);
    va_end(args);
    PrintHelper(buff);
  }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
  {
    Print("***Unhandled Exception:***\n");
    Print("Exception: %08x", (ezUInt32)pExceptionInfo->ExceptionRecord->ExceptionCode);

    {
      Print("\n\n***Stack Trace:***\n");
      void* pBuffer[64];
      ezArrayPtr<void*> tempTrace(pBuffer);
      const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace, pExceptionInfo->ContextRecord);

      ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
    }
    return EXCEPTION_CONTINUE_SEARCH;
  }
#else if EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  void TopLevelExceptionHandler() noexcept
  {
    Print("***Unhandled Exception:***\n");

    // Print exception type
    if (std::type_info* type = abi::__cxa_current_exception_type())
    {
      if (const char* szName = type->name())
      {
        int status = -1;
        // Try to print nice name
        if (char* szNiceName = abi::__cxa_demangle(szName, 0, 0, &status))
          Print("Exception: %s\n", szNiceName);
        else
          Print("Exception: %s\n", szName);
      }
    }

    {
      Print("\n\n***Stack Trace:***\n");
      void* pBuffer[64];
      ezArrayPtr<void*> tempTrace(pBuffer);
      const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

      ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
    }
    std::_Exit(EXIT_FAILURE);
  }

  static const auto g_pOldExceptionHandler = std::set_terminate(TopLevelExceptionHandler);

  void SignalHandler(int signal)
  {
    Print("***Unhandled Signal:***\n");
    switch (signal)
    {
      case SIGINT:
        Print("Signal SIGINT: interrupt\n");
        break;
      case SIGILL:
        Print("Signal SIGILL: illegal instruction - invalid function image\n");
        break;
      case SIGFPE:
        Print("Signal SIGFPE: floating point exception\n");
        break;
      case SIGSEGV:
        Print("Signal SIGSEGV: segment violation\n");
        break;
      case SIGTERM:
        Print("Signal SIGTERM: Software termination signal from kill\n");
        break;
      case SIGABRT:
        Print("Signal SIGABRT: abnormal termination triggered by abort call\n");
        break;
      default:
        Print("Signal %i: unknown signal\n", signal);
        break;
    }

    {
      Print("\n\n***Stack Trace:***\n");
      void* pBuffer[64];
      ezArrayPtr<void*> tempTrace(pBuffer);
      const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

      ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
    }
    std::_Exit(EXIT_FAILURE);
  }

#endif
}

int ezTestSetup::s_argc = 0;
const char** ezTestSetup::s_argv = nullptr;

ezTestFramework* ezTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int argc, const char** argv)
{
  s_argc = argc;
  s_argv = argv;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  SetUnhandledExceptionFilter(ExceptionHandler::TopLevelExceptionHandler);
#else
  std::signal(SIGINT, ExceptionHandler::SignalHandler);
  std::signal(SIGILL, ExceptionHandler::SignalHandler);
  std::signal(SIGFPE, ExceptionHandler::SignalHandler);
  std::signal(SIGSEGV, ExceptionHandler::SignalHandler);
  std::signal(SIGTERM, ExceptionHandler::SignalHandler);
  std::signal(SIGABRT, ExceptionHandler::SignalHandler);
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
  QApplication app(argc, argv);

  app.setApplicationName(pTestFramework->GetTestName());

  ezQtTestGUI::SetDarkTheme();
  // Create main window
  ezQtTestGUI mainWindow(*static_cast<ezQtTestFramework*>(pTestFramework));
  mainWindow.show();

  app.exec();

  return ezTestAppRun::Quit;
#elif EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  static_cast<ezUwpTestFramework*>(pTestFramework)->Run();
  return ezTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTestExecutionLoop();
#endif
}

void ezTestSetup::DeInitTestFramework()
{
  ezTestFramework* pTestFramework = ezTestFramework::GetInstance();

  ezStartup::ShutdownCore();

  // In the UWP case we never initialized this thread for ez, so we can't do log output now.
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezStringUtils::PrintStringLengthStatistics();
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
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
