#pragma once

#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

#include <Foundation/Threading/Thread.h>
#include <Foundation/Threading/ThreadSignal.h>

/// \internal Internal task worker thread class.
class ezTaskWorkerThread final : public ezThread
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTaskWorkerThread);

  /// \name Execution
  ///@{

public:
  /// \brief Tells the worker thread what tasks to execute and which thread index it has.
  ezTaskWorkerThread(ezWorkerThreadType::Enum threadType, ezUInt32 uiThreadNumber);
  ~ezTaskWorkerThread();

  /// \brief Deactivates the thread. Returns failure, if the thread is currently still running.
  ezResult DeactivateWorker();

  /// \brief Broadcasts ezThreadEvent::ClearThreadLocals on this thread.
  void BroadcastClearThreadLocalsEvent();

  void WaitForBroadcastClearTLS();

private:
  // Which types of tasks this thread should work on.
  ezWorkerThreadType::Enum m_WorkerType;

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive = true;
  bool m_bClearThreadLocalsEvent = false;

  // For display purposes.
  ezUInt16 m_uiWorkerThreadNumber = 0xFFFF;

  ///@}

  /// \name Thread Utilization
  ///@{

public:
  /// \brief Returns the last utilization value (0 - 1 range). Optionally returns how many tasks it executed recently.
  double GetThreadUtilization(ezUInt32* pNumTasksExecuted = nullptr);

  /// \brief Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void UpdateThreadUtilization(ezTime timePassed);

private:
  bool m_bExecutingTask = false;
  ezUInt16 m_uiLastNumTasksExecuted = 0;
  ezUInt16 m_uiNumTasksExecuted = 0;
  ezTime m_StartedWorkingTime;
  ezTime m_ThreadActiveTime;
  double m_fLastThreadUtilization = 0.0;

  ///@}

  /// \name Idle State
  ///@{

public:
  /// \brief If the thread is currently idle, this will wake it up and return EZ_SUCCESS.
  ezTaskWorkerState WakeUpIfIdle();

private:
  // Puts the thread to sleep (idle state)
  void WaitForWork();

  virtual ezUInt32 Run() override;

  // used to wake up idle threads, see m_WorkerState
  ezThreadSignal m_WakeUpSignal;

  // used to indicate whether this thread is currently idle
  // if so, it can be woken up using m_WakeUpSignal
  // ezAtomicBool m_bIsIdle = false;
  ezAtomicInteger32 m_iWorkerState; // ezTaskWorkerState

  ///@}
};

/// \internal Thread local state used by the task system (and for better debugging)
struct ezTaskWorkerInfo
{
  ezWorkerThreadType::Enum m_WorkerType = ezWorkerThreadType::Unknown;
  bool m_bAllowNestedTasks = true;
  ezInt32 m_iWorkerIndex = -1;
  const char* m_szTaskName = nullptr;
  ezAtomicInteger32* m_pWorkerState = nullptr;
};

extern thread_local ezTaskWorkerInfo tl_TaskWorkerInfo;
