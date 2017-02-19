#pragma once

#include <Foundation/Basics.h>

/// \brief Helper class to capture the current stack and print a captured stack
class EZ_FOUNDATION_DLL ezStackTracer
{
public:
  /// \brief Captures the current stack trace.
  ///
  /// The trace will contain not more than trace.GetCount() entries.
  /// If called in an exception handler, set bWithinException to true.
  /// Returns the actual number of captured entries.
  static ezUInt32 GetStackTrace(ezArrayPtr<void*>& trace, bool bWithinException = false);

  /// \brief Callback-function to print a text somewhere
  typedef void (*PrintFunc)(const char* szText);

  /// \brief Print a stack trace
  static void ResolveStackTrace(const ezArrayPtr<void*>& trace, PrintFunc printFunc);
};

