#pragma once

#include <Foundation/Types/Id.h>

class ezStreamWriter;
class ezThread;

/// \brief This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE provided below.
class EZ_FOUNDATION_DLL ezProfilingScope
{
public:
  ezProfilingScope(const char* szName, const char* szFunctionName);

  ~ezProfilingScope();

protected:
  const char* m_szName;
  ezUInt32 m_uiNameLength;
};

/// \brief Helper functionality of the profiling system.
class EZ_FOUNDATION_DLL ezProfilingSystem
{
public:
  /// \brief This is implementation specific. The default profiling captures the current data and writes it as json to the output stream. GPA does nothing.
  static void Capture(ezStreamWriter& outputStream);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend ezUInt32 RunThread(ezThread* pThread);

  static void Initialize();
  /// \brief Removes profiling data of dead threads.
  static void Reset();

  /// \brief Sets the name of the current thread.
  static void SetThreadName(const char* szThreadName);
  /// \brief Removes the current thread from the profiling system.
  ///  Needs to be called before the thread exits to be able to release profiling memory of dead threads on Reset.
  static void RemoveThread();
};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Profiles the current scope using the given name. The name string must not be destroyed before the current scope ends.
#define EZ_PROFILE(szName) \
  ezProfilingScope EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(szName, EZ_SOURCE_FUNCTION)

#else

#define EZ_PROFILE(Name)

#endif

