#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

/// \brief Helper class to capture the current stack and print a captured stack
class EZ_FOUNDATION_DLL ezStackTracer
{
public:
  /// \brief Captures the current stack trace.
  ///
  /// The trace will contain not more than trace.GetCount() entries.
  /// [Windows] If called in an exception handler, set pContext to PEXCEPTION_POINTERS::ContextRecord.
  /// Returns the actual number of captured entries.
  static ezUInt32 GetStackTrace(ezArrayPtr<void*>& trace, void* pContext = nullptr);

  /// \brief Callback-function to print a text somewhere
  typedef void (*PrintFunc)(const char* szText);

  /// \brief Print a stack trace
  static void ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc);

private:
  static void OnPluginEvent(const ezPlugin::PluginEvent& e);

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, StackTracer);
};

