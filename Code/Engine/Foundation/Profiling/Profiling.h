#pragma once

#include <Foundation/Basics/Types/Id.h>

class ezProfilingId;
class ezThread;

/// \brief This class encapsulates a profiling scope.
///
/// The constructor creates a new scope in the profiling system and the destructor pops the scope.
/// You shouldn't need to use this directly, just use the macro EZ_PROFILE provided below.
class EZ_FOUNDATION_DLL ezProfilingScope
{
public:
  ezProfilingScope(const ezProfilingId& id, const char* szFileName, const char* szFunctionName, 
    ezUInt32 uiLineNumber);

  ~ezProfilingScope();

private:
  const ezProfilingId& m_Id;
};

/// \brief Helper functionality of the profiling system.
struct EZ_FOUNDATION_DLL ezProfilingSystem
{
private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend class ezProfilingId;
  friend ezUInt32 RunThread(ezThread* pThread);
  
  /// \brief Initialization functionality of the threading system (called by foundation startup and thus private).
  static void Initialize();

  /// \brief Cleanup functionality of the threading system (called by foundation shutdown and thus private).
  static void Shutdown();

  /// \brief Registers a new id.
  static ezId24 RegisterId(const char* szName);

  /// \brief Frees the storage for an id so it can be reused.
  static void DeregisterId(ezId24 id);

  /// \brief Sets the name of the current thread.
  static void SetThreadName(const char* szThreadName);
};

#if EZ_ENABLED(EZ_USE_PROFILING)

/// \brief Small helper class to represent a profiling id.
///
/// The constructor assigns a free id and the destructor gives this id back for re-use.
/// Please note that id creation and deletion is an expensive operation and should be avoided during runtime.
/// Also keep in mind that the initialization of local static variables is NOT thread safe on all compilers, 
/// therefore it can lead to undefined behavior when creating a profiling id as local static.
class ezProfilingId
{
public:
  EZ_FORCE_INLINE ezProfilingId()
  {
  }

  EZ_FORCE_INLINE explicit ezProfilingId(const char* szName) : 
    m_Id(ezProfilingSystem::RegisterId(szName))
  {    
  }

  EZ_FORCE_INLINE ~ezProfilingId()
  {
    ezProfilingSystem::DeregisterId(m_Id);
  }
  
private:
  friend class ezProfilingScope;

  ezId24 m_Id;
};

/// \brief Profiles the current scope using the given profiling Id. The id must not be destroyed before the current scope ends.
#define EZ_PROFILE(Id) \
  ezProfilingScope EZ_CONCAT(_ezProfilingScope, EZ_SOURCE_LINE)(Id, \
    EZ_SOURCE_FILE, EZ_SOURCE_FUNCTION, EZ_SOURCE_LINE)

#else

class ezProfilingId
{
public:
  EZ_FORCE_INLINE ezProfilingId() { }
  EZ_FORCE_INLINE explicit ezProfilingId(const char* szName) { }
};

#define EZ_PROFILE(Name)

#endif
