#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Thread.h>

/// \brief This class is the base class for threads which need dispatching of calls.
///
/// Used by deriving from this class and overriding the Run() method. Call DispatchQueue() regurarely so that the collected messages can be
/// dispatched.
class EZ_FOUNDATION_DLL ezThreadWithDispatcher : public ezThread
{
public:
  using DispatchFunction = ezDelegate<void(), 128>;

  /// \brief Initializes the runnable class
  ezThreadWithDispatcher(const char* szName = "ezThreadWithDispatcher", ezUInt32 uiStackSize = 128 * 1024);

  /// \brief Destructor checks if the thread is deleted while still running, which is not allowed as this is a data hazard
  virtual ~ezThreadWithDispatcher();

  /// \brief Use this to enqueue a function call to the given delegate at some later point running in the given thread context.
  void Dispatch(DispatchFunction&& pDelegate);

protected:
  /// \brief Needs to be called by derived thread implementations to dispatch the function calls.
  void DispatchQueue();

private:
  /// \brief The run function can be used to implement a long running task in a thread in a platform independent way
  virtual ezUInt32 Run() = 0;

  ezDynamicArray<DispatchFunction> m_ActiveQueue;
  ezDynamicArray<DispatchFunction> m_CurrentlyBeingDispatchedQueue;

  ezMutex m_QueueMutex;
};
