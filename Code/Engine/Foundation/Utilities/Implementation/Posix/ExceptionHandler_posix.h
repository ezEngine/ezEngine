
#include <csignal>
#include <cxxabi.h>

void ezExceptionHandler::DefaultExceptionHandler() noexcept
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
  WriteDump();
  std::_Exit(EXIT_FAILURE);
}

namespace ExceptionHandler
{
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
    WriteDump();
    std::_Exit(EXIT_FAILURE);
  }
} // namespace ExceptionHandler

void ezExceptionHandler::SetExceptionHandler(EZ_TOP_LEVEL_EXCEPTION_HANDLER handler, const char* appName, const char* absDumpPath)
{
  s_appName = appName;
  s_absDumpPath = absDumpPath;

  std::signal(SIGINT, ExceptionHandler::SignalHandler);
  std::signal(SIGILL, ExceptionHandler::SignalHandler);
  std::signal(SIGFPE, ExceptionHandler::SignalHandler);
  std::signal(SIGSEGV, ExceptionHandler::SignalHandler);
  std::signal(SIGTERM, ExceptionHandler::SignalHandler);
  std::signal(SIGABRT, ExceptionHandler::SignalHandler);
  std::set_terminate(handler);
}

ezResult ezExceptionHandler::WriteDump()
{
  Print("Writing dumps not implemented.");
  return EZ_FAILURE;
}
