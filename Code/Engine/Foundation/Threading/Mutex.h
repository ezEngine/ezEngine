#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief Provides a simple mechanism for mutual exclusion to prevent multiple threads from accessing a shared resource simultaneously. 
///
/// This can be used to protect code that is not thread-safe against race conditions.
/// To ensure that mutexes are always properly released, use the scoped Lock class.
class ezMutex
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

class ezNoMutex
{
public:
  EZ_FORCE_INLINE void Acquire() { }
  EZ_FORCE_INLINE void Release() { }
};

#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/Mutex_win.h>
#elif EZ_PLATFORM_OSX
  #include <Foundation/Threading/Implementation/Posix/Mutex_posix.h>
#else
  #error "Mutex is not implemented on current platform"
#endif
