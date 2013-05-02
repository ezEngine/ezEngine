#pragma once

#include <Foundation/Basics.h>

/// \brief Implementation of a thread.
///
/// Since the thread class needs a platform specific entrypoint it is usually
/// recommended to use the ezThread class instead as the base for long running threads.
class EZ_FOUNDATION_DLL ezOSThread
{
public:

  /// \brief Initializes the thread instance (e.g. thread creation etc.)
  ///
  /// Note that the thread won't start execution until Start() is called.
  ezOSThread(ezOSThreadEntryPoint pThreadEntryPoint, void* pUserData = NULL, const char* szName = "ezOSThread", ezUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor.
  virtual ~ezOSThread();
  
  /// \brief Starts the thread
  void Start(); // [tested]
  
  /// \brief Waits in the calling thread until the thread has finished execution (e.g. returned from the thread function)
  void Join(); // [tested]

  /// \brief Returns the thread handle of the thread object, may be used in comparison operations with ezThreadUtils::GetCurrentThreadHandle() for example.
  inline const ezThreadHandle& GetThreadHandle() const
  {
    return m_Handle;
  }

protected:

  ezThreadHandle m_Handle;

  ezOSThreadEntryPoint m_EntryPoint;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezOSThread);
};
