#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief Implementation of a thread.
///
/// Since the thread class needs a platform specific entry-point it is usually
/// recommended to use the ezThread class instead as the base for long running threads.
class EZ_FOUNDATION_DLL ezOSThread
{
public:

  /// \brief Initializes the thread instance (e.g. thread creation etc.)
  ///
  /// Note that the thread won't start execution until Start() is called. Please note that szName must be valid until Start() has been called!
  ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData = nullptr, const char* szName = "ezOSThread", ezUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor.
  virtual ~ezOSThread();
  
  /// \brief Starts the thread
  void Start(); // [tested]
  
  /// \brief Waits in the calling thread until the thread has finished execution (e.g. returned from the thread function)
  void Join(); // [tested]

  /// \brief Returns the thread ID of the thread object, may be used in comparison operations with ezThreadUtils::GetCurrentThreadID() for example.
  const ezThreadID& GetThreadID() const
  {
    return m_ThreadID;
  }

  /// \brief Returns how many ezOSThreads are currently active.
  static ezInt32 GetThreadCount() { return s_iThreadCount; }

protected:

  ezThreadHandle m_Handle;
  ezThreadID m_ThreadID;

  ezOSThreadEntryPoint m_EntryPoint;
  
  void* m_pUserData;
  
  const char* m_szName;
  
  ezUInt32 m_uiStackSize;


private:

  /// Stores how many ezOSThread are currently active.
  static ezAtomicInteger32 s_iThreadCount;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezOSThread);
};

