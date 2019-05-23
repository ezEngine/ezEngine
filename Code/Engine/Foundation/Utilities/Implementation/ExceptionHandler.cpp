#include <FoundationPCH.h>

#include <Foundation/Utilities/ExceptionHandler.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/StackTracer.h>

static void PrintHelper(const char* szText)
{
  printf("%s", szText);
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  OutputDebugStringW(ezStringWChar(szText).GetData());
#endif
  fflush(stdout);
  fflush(stderr);
};

template<typename... Args>
static void Print(const char* szFormat, Args... args)
{
  char buff[1024];
  ezStringUtils::snprintf(buff, EZ_ARRAY_SIZE(buff), szFormat, args...);
  PrintHelper(buff);
}

ezString ezExceptionHandler::s_appName;
ezString ezExceptionHandler::s_absDumpPath;

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Utilities/Implementation/Win/ExceptionHandler_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/Utilities/Implementation/Posix/ExceptionHandler_posix.h>
#else
#  error "ExceptionHandler is not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_ExceptionHandler);

