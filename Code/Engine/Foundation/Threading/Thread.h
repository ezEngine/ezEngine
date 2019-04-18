#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

#define EZ_MSVC_WARNING_NUMBER 4355
#include <Foundation/Basics/Compiler/DisableWarning.h>

#ifndef EZ_THREAD_CLASS_ENTRY_POINT
#  error "Definition for ezThreadClassEntryPoint is missing on this platform!"
#endif

EZ_THREAD_CLASS_ENTRY_POINT;

struct ezThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the ezThread instance
    ThreadDestroyed,   ///< Called on the thread that destroys the ezThread instance
    StartingExecution, ///< Called on the thread that executes the ezThread instance
    FinishedExecution, ///< Called on the thread that executes the ezThread instance
  };

  Type m_Type;
  ezThread* m_pThread = nullptr;
};

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
  ezThread(const char* szName = "ezThread", ezUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~ezThread();

  /// \brief Returns the thread status
  inline ezThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// \brief Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// \brief Returns the thread name
  inline const char* GetThreadName() const { return m_Name.GetData(); }

  /// \brief These events inform about threads starting and finishing.
  ///
  /// The events are raised on the executing thread! That means thread-specific code may be executed during the event callback,
  /// e.g. to set up thread-local functionality. To not introduce unnecessary bottlenecks, the events are raised without a mutex,
  /// so ensure make the called code thread-safe.
  static ezEvent<const ezThreadEvent&, ezNoMutex> s_ThreadEvents;

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual ezUInt32 Run() = 0;


  volatile ezThreadStatus m_ThreadStatus;

  ezString m_Name;

  friend ezUInt32 RunThread(ezThread* pThread);
};

#include <Foundation/Basics/Compiler/RestoreWarning.h>
