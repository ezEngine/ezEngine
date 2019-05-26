#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
typedef LONG(WINAPI* EZ_TOP_LEVEL_EXCEPTION_HANDLER)(struct _EXCEPTION_POINTERS* pExceptionInfo);
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
typedef void(* EZ_TOP_LEVEL_EXCEPTION_HANDLER)();
#else
#  error "ezExceptionHandler is not implemented on current platform"
#endif

/// \brief Helper class to manage the top level exception handler.
///
/// A default exception handler is provided but not set by default.
/// The default implementation will write the exception and callstack to the output
/// and create a memory dump using WriteDump that create a dump file in the folder
/// specified via SetExceptionHandler.
class EZ_FOUNDATION_DLL ezExceptionHandler
{
public:
  /// \brief Sets the the global top level exception handler.
  /// \param handler The exception handler to set. Any previously set will be overwritten.
  /// \param appName Name of the application. Pre-pended to memory dump file names.
  /// \param absDumpPath Absolute path to store memory dumps in.
  static void SetExceptionHandler(EZ_TOP_LEVEL_EXCEPTION_HANDLER handler, const char* appName, const char* absDumpPath);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  static LONG WINAPI DefaultExceptionHandler(struct _EXCEPTION_POINTERS* pExceptionInfo);
  static ezResult WriteDump(EXCEPTION_POINTERS* exceptionInfo = nullptr);
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  static void DefaultExceptionHandler() noexcept;
  static ezResult WriteDump();
#else
#  error "ezExceptionHandler is not implemented on current platform"
#endif

private:
  static ezString s_appName;
  static ezString s_absDumpPath;
};
