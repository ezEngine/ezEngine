#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Threading/Implementation/OSThread.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completly initialized object happen)

#define EZ_MSVC_WARNING_NUMBER 4355
#include <Foundation/Basics/Compiler/DisableWarning.h>

#ifndef EZ_THREAD_CLASS_ENTRY_POINT
  #error "Definition for ezThreadClassEntryPoint is missing on this platform!"
#endif

EZ_THREAD_CLASS_ENTRY_POINT;

/// This class is the base class for platform independent long running threads
/// Used by deriving from this class and overriding the Run() method.
class EZ_FOUNDATION_DLL ezThread : public ezOSThread
{
public:

  /// Describes the thread status
  enum ezThreadStatus
  {
    Created = 0,
    Running,
    Finished
  };

  /// Initializes the runnable class
  ezThread(const char* pName = "ezThread", ezUInt32 uiStackSize = 128 * 1024)
    : ezOSThread(ezThreadClassEntryPoint, this, pName, uiStackSize), m_ThreadStatus(Created)
  {
  }

  /// Destructor (used to check if the thread is deleted while still running, which is not allowed as this is a data hazard)
  virtual ~ezThread()
  {
    EZ_ASSERT(!IsRunning(), "Thread deletion while still running detected!");
  }


  /// The run function can be used to implement a long running task in a thread
  /// in a platform independent way
  virtual ezUInt32 Run() = 0;

  /// Returns the thread status
  ezThreadStatus GetThreadStatus() const
  {
    return m_ThreadStatus;
  }

  /// Helper function to wait until the thread started its work or even finished execution
  void WaitUntilRunningOrFinished() const
  {
    while(m_ThreadStatus == Created)
    {
    }
  }
  
  /// Helper function to determine if the thread is running
  bool IsRunning() const
  {
    return m_ThreadStatus == Running;
  }

private:

  volatile ezThreadStatus m_ThreadStatus;

  ezThreadLocalPointerTable m_ThreadLocalPointerTable;
  
  friend EZ_THREAD_CLASS_ENTRY_POINT;

};

#include <Foundation/Basics/Compiler/RestoreWarning.h>