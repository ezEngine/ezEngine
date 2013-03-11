#pragma once

#include <Foundation/Basics.h>

/// This class encapsulates a profiling scope, the constructor creates a new scope
/// in the profiling system and the destructor pops the scope
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE provided below.
class EZ_FOUNDATION_DLL ezProfilingScope
{
public:

  inline ezProfilingScope(const char* pszSampleName, const char* pszFileName, const char* pszFunctionName, const ezUInt32 uiLineNumber);

  inline ~ezProfilingScope();

};

#if defined(EZ_PROFILING_ENABLED)

/// Profiles a scope, please note that Name is used as part of a variable name and thus has to be a valid C++ identifier.
#define EZ_PROFILE(Name) ezProfilingScope _ezProfilingScope##Name_(#Name, EZ_SOURCE_FILE, EZ_SOURCE_FUNCTION, EZ_SOURCE_LINE)

#else
  #define EZ_PROFILE(Name)
#endif

/// Helper functionality of the profiling system
struct EZ_FOUNDATION_DLL ezProfilingSystem
{
private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  
  // Initialization functionality of the threading system (called by foundation startup and thus private)
  static void Initialize();

  // Cleanup functionality of the threading system (called by foundation shutdown and thus private)
  static void Shutdown();
};


