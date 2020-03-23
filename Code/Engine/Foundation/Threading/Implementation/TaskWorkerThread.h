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
  ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 uiThreadNumber);
  ~ezTaskWorkerThread();

  /// \brief Deactivates the thread. Returns failure, if the thread is currently still running.
  ezResult DeactivateWorker();

private:
  // Which types of tasks this thread should work on.
  ezWorkerThreadType::Enum m_WorkerType;

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive = true;

  // For display purposes.
  ezUInt16 m_uiWorkerThreadNumber = 0xFFFF;

  ///@}

  /// \name Thread Utilization
  ///@{

public:
  /// \brief Returns the last utilization value (0 - 1 range). Optionally returns how many tasks it executed recently.
  double GetThreadUtilization(ezUInt32* pNumTasksExecuted = nullptr);

  /// \brief Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void UpdateThreadUtilization(ezTime TimePassed);

private:
  bool m_bExecutingTask = false;
  ezUInt16 m_uiLastNumTasksExecuted = 0;
  ezTime m_StartedWorkingTime;
  ezTime m_ThreadActiveTime;
  ezAtomicInteger32 m_iNumTasksExecuted;
  double m_fLastThreadUtilization = 0.0;

  ///@}

  /// \name Idle State
  ///@{

public:

  /// \brief If the thread is currently idle, this will wake it up and return EZ_SUCCESS.
  ezResult WakeUpIfIdle();

private:

  // Puts the thread to sleep (idle state)
  void WaitForWork();

  virtual ezUInt32 Run() override;

  // used to wake up idle threads, see m_bIsIdle
  ezThreadSignal m_WakeUpSignal;

  // used to indicate whether this thread is currently idle
  // if so, it can be woken up using m_WakeUpSignal
  ezAtomicBool m_bIsIdle = false;

  ///@}
};
