#pragma once

#include <Foundation/Basics.h>

/// \brief Helper class to capture the current stack and print a captured stack 
class EZ_FOUNDATION_DLL ezStackTracer
{
public:
  /// \brief Captures the current stack trace.
  ///
  /// The trace will contain not more than trace.GetCount() entries. Returns the actual number of captured entries.
  static ezUInt32 GetStackTrace(ezArrayPtr<void*>& trace);

  /// \brief Print a stack trace
  static void DumpStackTrace(const ezArrayPtr<void*>& trace);
};

