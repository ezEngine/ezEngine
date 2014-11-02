#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief Provides a simple mechanism for mutual exclusion to prevent multiple threads from accessing a shared resource simultaneously. 
///
/// This can be used to protect code that is not thread-safe against race conditions.
/// To ensure that mutexes are always properly released, use the scoped Lock class.
class EZ_FOUNDATION_DLL ezMutex
{
public:
  ezMutex();
  ~ezMutex();
  
  /// \brief Attempts to acquire an exclusive lock for this mutex object
  void Acquire();
  
  /// \brief Releases a lock that has been previously acquired
  void Release();

private:
  ezMutexHandle m_Handle;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezMutex);
};

/// \brief A dummy mutex that does no locking.
///
/// Used when a mutex object needs to be passed to some code (such as allocators), but thread-synchronization
/// is actually not necessary.
class EZ_FOUNDATION_DLL ezNoMutex
{
public:

  /// \brief Implements the 'Acquire' interface function, but does nothing.
  EZ_FORCE_INLINE void Acquire() { }

  /// \brief Implements the 'Release' interface function, but does nothing.
  EZ_FORCE_INLINE void Release() { }
};

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/Mutex_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/Mutex_posix.h>
#else
  #error "Mutex is not implemented on current platform"
#endif

