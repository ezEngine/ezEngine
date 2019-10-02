#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/StackTracer.h>

#include <csignal>
#include <cxxabi.h>
#include <unistd.h>

static void ezCrashHandlerFunc() noexcept
{
  if (ezCrashHandler::GetCrashHandler() != nullptr)
  {
    ezCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // restore the original signal handler for the abort signal and raise one so the kernel can do a core dump
  std::signal(SIGABRT, SIG_DFL);
  std::raise(SIGABRT);
}

static void ezSignalHandler(int signum)
{
  ezLog::Printf("***Unhandled Signal:***\n");
  switch (signum)
  {
    case SIGINT:
      ezLog::Printf("Signal SIGINT: interrupt\n");
      break;
    case SIGILL:
      ezLog::Printf("Signal SIGILL: illegal instruction - invalid function image\n");
      break;
    case SIGFPE:
      ezLog::Printf("Signal SIGFPE: floating point exception\n");
      break;
    case SIGSEGV:
      ezLog::Printf("Signal SIGSEGV: segment violation\n");
      break;
    case SIGTERM:
      ezLog::Printf("Signal SIGTERM: Software termination signal from kill\n");
      break;
    case SIGABRT:
      ezLog::Printf("Signal SIGABRT: abnormal termination triggered by abort call\n");
      break;
    default:
      ezLog::Printf("Signal %i: unknown signal\n", signal);
      break;
  }

  if (ezCrashHandler::GetCrashHandler() != nullptr)
  {
    ezCrashHandler::GetCrashHandler()->HandleCrash(nullptr);
  }

  // forward the signal back to the OS so that it can write a core dump
  std::signal(signum, SIG_DFL);
  kill(getpid(), signum);
}

void ezCrashHandler::SetCrashHandler(ezCrashHandler* pHandler)
{
  s_pActiveHandler = pHandler;

  if (s_pActiveHandler != nullptr)
  {
    std::signal(SIGINT, ezSignalHandler);
    std::signal(SIGILL, ezSignalHandler);
    std::signal(SIGFPE, ezSignalHandler);
    std::signal(SIGSEGV, ezSignalHandler);
    std::signal(SIGTERM, ezSignalHandler);
    std::signal(SIGABRT, ezSignalHandler);
    std::set_terminate(ezCrashHandlerFunc);
  }
  else
  {
    std::signal(SIGINT, nullptr);
    std::signal(SIGILL, nullptr);
    std::signal(SIGFPE, nullptr);
    std::signal(SIGSEGV, nullptr);
    std::signal(SIGTERM, nullptr);
    std::signal(SIGABRT, nullptr);
    std::set_terminate(nullptr);
  }
}

bool ezCrashHandler_WriteMiniDump::WriteOwnProcessMiniDump(void* pOsSpecificData)
{
  return false;
}

void ezCrashHandler_WriteMiniDump::PrintStackTrace(void* pOsSpecificData)
{
  ezLog::Printf("***Unhandled Exception:***\n");

  // ezLog::Printf exception type
  if (std::type_info* type = abi::__cxa_current_exception_type())
  {
    if (const char* szName = type->name())
    {
      int status = -1;
      // Try to print nice name
      if (char* szNiceName = abi::__cxa_demangle(szName, 0, 0, &status))
        ezLog::Printf("Exception: %s\n", szNiceName);
      else
        ezLog::Printf("Exception: %s\n", szName);
    }
  }

  {
    ezLog::Printf("\n\n***Stack Trace:***\n");

    void* pBuffer[64];
    ezArrayPtr<void*> tempTrace(pBuffer);
    const ezUInt32 uiNumTraces = ezStackTracer::GetStackTrace(tempTrace);

    ezStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &PrintHelper);
  }
}
