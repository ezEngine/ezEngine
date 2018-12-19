#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Time/Time.h>

class ezStreamWriter;
class ezThread;

/// \brief This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE_SCOPE provided below.
class EZ_FOUNDATION_DLL ezProfilingScope
{
public:
  ezProfilingScope(const char* szName, const char* szFunctionName);
  ~ezProfilingScope();

protected:
  const char* m_szName;
  ezUInt32 m_uiNameLength;
};

/// \brief This class implements a profiling scope similar to ezProfilingScope, but with additional sub-scopes which can be added easily without introducing actual C++ scopes.
///
/// The constructor pushes one surrounding scope on the stack and then a nested scope as the first section.
/// The function StartNextSection() will end the nested scope and start a new inner scope.
/// This allows to end one scope and start a new one, without having to add actual C++ scopes for starting/stopping profiling scopes.
///
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE_LIST_SCOPE provided below.
class ezProfilingListScope
{
public:
  EZ_FOUNDATION_DLL ezProfilingListScope(const char* szListName, const char* szFirstSectionName, const char* szFunctionName);
  EZ_FOUNDATION_DLL ~ezProfilingListScope();

  EZ_FOUNDATION_DLL static void StartNextSection(const char* szNextSectionName);

protected:
  static thread_local ezProfilingListScope* s_pCurrentList;

  ezProfilingListScope* m_pPreviousList;

  const char* m_szListName;
  ezUInt32 m_uiListNameLength;

  const char* m_szCurSectionName;
  ezUInt32 m_uiCurSectionNameLength;
};

/// \brief Helper functionality of the profiling system.
class EZ_FOUNDATION_DLL ezProfilingSystem
{
public:
  /// \brief This is implementation specific. The default profiling captures the current data and writes it as JSON to the output stream.
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

public:

  /// \brief Helper struct to hold GPU profiling data.
  struct GPUData
  {
    ezTime m_BeginTime;
    ezTime m_EndTime;
    char m_szName[48];
  };

  /// \brief Initialized internal data structures for GPU profiling data. Needs to be called before adding any data.
  static void InitializeGPUData();

  /// \brief Allocates GPU profiling data in the internal event ringbuffer.
  static GPUData& AllocateGPUData();

};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Profiles the current scope using the given name.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingScope
/// \sa EZ_PROFILE_LIST_SCOPE
#  define EZ_PROFILE_SCOPE(szScopeName) ezProfilingScope EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(szScopeName, EZ_SOURCE_FUNCTION)

/// \brief Profiles the current scope using the given name as the overall list scope name and the section name for the first section in the list.
///
/// Use EZ_PROFILE_LIST_NEXT_SECTION to start a new section in the list scope.
///
/// It is allowed to nest EZ_PROFILE_SCOPE, also with EZ_PROFILE_LIST_SCOPE. However EZ_PROFILE_SCOPE should start and end within the same list scope section.
///
/// \note The name string must not be destroyed before the current scope ends.
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_NEXT_SECTION
#  define EZ_PROFILE_LIST_SCOPE(szListName, szFirstSectionName)                                                                            \
    ezProfilingListScope EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(szListName, szFirstSectionName, EZ_SOURCE_FUNCTION)

/// \brief Starts a new section in a EZ_PROFILE_LIST_SCOPE
///
/// \sa ezProfilingListScope
/// \sa EZ_PROFILE_LIST_SCOPE
#  define EZ_PROFILE_LIST_NEXT_SECTION(szNextSectionName) ezProfilingListScope::StartNextSection(szNextSectionName)

#else

#  define EZ_PROFILE_SCOPE(Name) /*empty*/

#  define EZ_PROFILE_LIST_SCOPE(szListName, szFirstSectionName) /*empty*/

#  define EZ_PROFILE_LIST_NEXT_SECTION(szNextSectionName) /*empty*/

#endif
