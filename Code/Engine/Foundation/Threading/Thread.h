#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Threading/ThreadUtils.h>

#include <Foundation/Threading/Implementation/OSThread.h>

// Warning: 'this' used in member initialization list (is fine here since it is just stored and not
// accessed in the constructor (so no operations on a not completely initialized object happen)

EZ_WARNING_PUSH()
EZ_WARNING_DISABLE_MSVC(4355)

#ifndef EZ_THREAD_CLASS_ENTRY_POINT
#  error "Definition for ezThreadClassEntryPoint is missing on this platform!"
#endif

EZ_THREAD_CLASS_ENTRY_POINT;

struct ezThreadEvent
{
  enum class Type
  {
    ThreadCreated,     ///< Called on the thread that creates the ezThread instance (not the ezThread itself).
    ThreadDestroyed,   ///< Called on the thread that destroys the ezThread instance (not the ezThread itself).
    StartingExecution, ///< Called on the ezThread before the Run() method is executed.
    FinishedExecution, ///< Called on the ezThread after the Run() method was executed.
    ClearThreadLocals, ///< Potentially called on the ezThread (currently only for task system threads) at a time when plugins should clean up thread-local storage.
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
  /// \brief Returns the current ezThread if the current platform thread is an ezThread. Returns nullptr otherwise.
  static const ezThread* GetCurrentThread();

  /// \brief Describes the thread status
  enum ezThreadStatus
  {
    Created = 0,
    Running,
    Finished
  };

  /// \brief Initializes the runnable class
  ezThread(ezStringView sName = "ezThread", ezUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~ezThread();

  /// \brief Returns the thread status
  inline ezThreadStatus GetThreadStatus() const { return m_ThreadStatus; }

  /// \brief Helper function to determine if the thread is running
  inline bool IsRunning() const { return m_ThreadStatus == Running; }

  /// \brief Returns the thread name
  inline const char* GetThreadName() const { return m_sName.GetData(); }

  /// \brief These events inform about threads starting and finishing.
  ///
  /// The events are raised on the executing thread! That means thread-specific code may be executed during the event callback,
  /// e.g. to set up thread-local functionality.
  static ezEvent<const ezThreadEvent&, ezMutex> s_ThreadEvents;

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual ezUInt32 Run() = 0;


  volatile ezThreadStatus m_ThreadStatus = Created;

  ezString m_sName;

  friend ezUInt32 RunThread(ezThread* pThread);
};

EZ_WARNING_POP()
