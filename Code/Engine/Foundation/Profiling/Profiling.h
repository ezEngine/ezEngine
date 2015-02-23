#pragma once

#include <Foundation/Types/Id.h>

class ezStreamWriterBase;
class ezThread;
class ezProfilingId;

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

#define EZ_PROFILING_ID_COUNT 512

/// \brief Helper functionality of the profiling system.
class EZ_FOUNDATION_DLL ezProfilingSystem
{
public:
  /// \brief Registers a new id.
  static ezProfilingId CreateId(const char* szName);

  /// \brief Frees the storage for an id so it can be reused.
  static void DeleteId(const ezProfilingId& id);

  /// \brief This is implementation specific. The default profiling captures the current data and writes it as json to the output stream. GPA does nothing.
  static void Capture(ezStreamWriterBase& outputStream);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ProfilingSystem);
  friend class ezProfilingId;
  friend ezUInt32 RunThread(ezThread* pThread);

  static void Initialize();

  static void AddReference(const ezProfilingId& id);
  static void ReleaseReference(const ezProfilingId& id);

  /// \brief Sets the name of the current thread.
  static void SetThreadName(const char* szThreadName);
};

#if EZ_ENABLED(EZ_USE_PROFILING) || defined(EZ_DOCS)

/// \brief Small helper class to represent a profiling id.
///
/// To create a new profiling id use ezProfilingSystem::CreateId. Once the id is not needed anymore call 
/// ezProfilingSystem::DeleteId so the memory can be re-used.
/// Please note that id creation and deletion is an expensive operation and should be avoided during runtime.
/// Also keep in mind that the initialization of local static variables is NOT thread safe on all compilers, 
/// therefore it can lead to undefined behavior when creating a profiling id as local static.
class ezProfilingId
{
public:
  typedef ezGenericId<24, 8> InternalId;

  EZ_FORCE_INLINE ezProfilingId()
  {
  }

  EZ_FORCE_INLINE ezProfilingId(const ezProfilingId& rhs)
  {
    *this = rhs;
  }

  EZ_FORCE_INLINE ~ezProfilingId()
  {
    if (m_Id != InternalId())
      ezProfilingSystem::ReleaseReference(*this);
  }

  EZ_FORCE_INLINE void operator=(const ezProfilingId& rhs)
  {
    ezProfilingSystem::ReleaseReference(*this);
    m_Id = rhs.m_Id;
    ezProfilingSystem::AddReference(*this);
  }
  
private:
  friend class ezProfilingScope;
  friend class ezProfilingSystem;

  EZ_FORCE_INLINE explicit ezProfilingId(InternalId id) : 
    m_Id(id)
  {
  }  

  InternalId m_Id;
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
  EZ_FORCE_INLINE ezProfilingId(const ezProfilingId& rhs) { }
  EZ_FORCE_INLINE void operator=(const ezProfilingId& rhs) { }
};

#define EZ_PROFILE(Name)

#endif

