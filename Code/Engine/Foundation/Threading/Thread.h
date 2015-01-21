#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

#define EZ_MSVC_WARNING_NUMBER 4355
#include <Foundation/Basics/Compiler/DisableWarning.h>

#ifndef EZ_THREAD_CLASS_ENTRY_POINT
  #error "Definition for ezThreadClassEntryPoint is missing on this platform!"
#endif

EZ_THREAD_CLASS_ENTRY_POINT;

/// \brief This class is the base class for platform independent long running threads
///
/// Used by deriving from this class and overriding the Run() method.
class EZ_FOUNDATION_DLL ezThread : public ezOSThread
{
public:

  /// \brief Describes the thread status
  enum ezThreadStatus
  {
    Created = 0,
    Running,
    Finished
  };

  /// \brief Initializes the runnable class
  ezThread(const char* szName = "ezThread", ezUInt32 uiStackSize = 128 * 1024)
    : ezOSThread(ezThreadClassEntryPoint, this, szName, uiStackSize), m_ThreadStatus(Created), m_Name(szName)
  {
  }

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~ezThread()
  {
    EZ_ASSERT_DEV(!IsRunning(), "Thread deletion while still running detected!");
  }

  /// \brief Returns the thread status
  inline ezThreadStatus GetThreadStatus() const
  {
    return m_ThreadStatus;
  }

  /// \brief Helper function to determine if the thread is running
  inline bool IsRunning() const
  {
    return m_ThreadStatus == Running;
  }

  /// \brief Returns the thread name
  inline const char* GetThreadName() const
  {
    return m_Name.GetData();
  }

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual ezUInt32 Run() = 0;


  volatile ezThreadStatus m_ThreadStatus;

  ezThreadLocalPointerTable m_ThreadLocalPointerTable;

  ezString m_Name;
  
  friend ezUInt32 RunThread(ezThread* pThread);
};

#include <Foundation/Basics/Compiler/RestoreWarning.h>

