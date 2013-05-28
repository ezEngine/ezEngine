#pragma once

#include <Foundation/Threading/Thread.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>

/// \brief Derive from this base class to implement custom tasks.
class EZ_FOUNDATION_DLL ezTask
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTask);

public:
  /// \brief Function type for callbacks when a task has been finished (or canceled).
  typedef void(*OnTaskFinished)(ezTask* pTask);

public:
  ezTask();
  virtual ~ezTask() {}

  /// \brief Sets an additional callback function to execute when the task is finished (or canceled).
  /// The most common use case for this is to deallocate the task at this time.
  void SetOnTaskFinished(OnTaskFinished Callback);

  /// \brief Returns whether the task has been finished. This includes being canceled.
  ///
  /// \note This function is only reliable when you KNOW that the task has not been reused.
  /// So that limits its usage to the time frame while the task is in use, and it should only
  /// be queried by code that actually manages when to reuse the task.
  /// If other code needs to be able to check whether a task is finished, you should give it
  /// the ezTaskGroupID of the task's group. That one can be used to query whether the group
  /// has finished, even minutes later.
  bool IsTaskFinished() const { return m_bIsFinished; }

  /// \brief Can be used inside an overrideen 'Execute' function to terminate execution prematurely.
  bool HasBeenCanceled() const { return m_bCancelExecution; }

private:
  /// \brief Override this function to generate a proper name for this task.
  ///
  /// Called by GetProfilingID to get a display string for profiling.
  virtual void GetTaskName(ezStringBuilder& sName) const = 0;

  /// \brief Override this to implement the task's supposed functionality.
  virtual void Execute() = 0;

private:
  // The task system and its worker threads implement most of the functionality of the task handling.
  // Therefore they are allowed to modify all this internal state.
  friend class ezTaskWorkerThread;
  friend class ezTaskSystem;

  /// \brief Called by ezTaskSystem to execute the task. Calls 'Execute' internally.
  void Run();

  /// \brief Allocates and returns a profiling ID for this task. Called by ezTaskSystem.
  const ezProfilingId& GetProfilingID();

  /// \brief Set to true once the task is finished or properly canceled.
  volatile bool m_bIsFinished;

  /// \brief Set to true when the task is SUPPOSED to cancel. Whether the task is able to do that, depends on its implementation.
  volatile bool m_bCancelExecution;

  /// \brief Just stores whether m_ProfilingID has been generated, to prevent doing it twice.
  bool m_bProfilingIDGenerated;

  /// \brief The profiling ID for this task.
  ezProfilingId m_ProfilingID;

  /// \brief Optional callback to be fired when the task has finished or was canceled.
  OnTaskFinished m_OnTaskFinished;
  
  /// \brief The parent group to which this task belongs.
  ezTaskGroupID m_BelongsToGroup;
};

/// \brief This system allows to automatically distribute tasks onto a number of worker threads.
///
/// 
class EZ_FOUNDATION_DLL ezTaskSystem
{
public:
  /// \brief Sets the number of threads to use for the different task categories.
  ///
  /// \a uiShortTasks and \a uiLongTasks must be at least 1 and should not exceed the number of available CPU cores.
  /// There will always be exactly one additional thread for file access tasks (ezTaskPriority::FileAccess).
  static void SetWorkThreadCount(ezUInt8 uiShortTasks, ezUInt8 uiLongTasks);

  /// \brief A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task has been put.
  static ezTaskGroupID StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority);

  /// \brief A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task has been put.
  /// This overload allows to additionally specify a single dependency.
  static ezTaskGroupID StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority, ezTaskGroupID Dependency);


  /// \brief Creates a new task group for one-time use. Groups need to be recreated every time a task is supposed to be inserted into the system.
  ///
  /// All tasks that are added to this group will be run with the same given \a Priority.
  /// Once all tasks in the group are finished and thus the group is finished, an optional \a Callback can be executed.
  static ezTaskGroupID CreateTaskGroup(ezTaskPriority::Enum Priority, ezTaskGroup::OnTaskGroupFinished Callback = NULL);

  /// \brief Adds a task to the given task group. The group must not yet have been started.
  static void AddTaskToGroup(ezTaskGroupID Group, ezTask* pTask);

  /// \brief Adds a dependency on another group to \a Group. This means \a Group will not be execute before \a DependsOn has finished.
  ///
  /// \note Be careful with dependencies and task priorities. A task that has to execute 'this frame' should never depend on a task
  /// that needs only finish 'next frame', this might introduce very long and unnecesary waits.
  /// A task that has priority 'this frame' or 'next frame' will actually not be executed in 'this frame' or 'next frame' until all
  /// its dependencies are fulfilled. So you might add a long running task and a short task which depends on it, but the system will
  /// not block at the end of the frame, to wait for the long running task (to finish the short task thereafter), as that short task
  /// won't get scheduled for execution, at all, until all its dependencies are actually finished.
  static void AddTaskGroupDependency(ezTaskGroupID Group, ezTaskGroupID DependsOn);

  /// \brief Starts the task group. After this no further modifications on the group (new tasks or dependencies) are allowed.
  static void StartTaskGroup(ezTaskGroupID Group);

  /// \brief Returns whether the given \a Group id refers to a task group that has been finished already.
  ///
  /// There is no time frame in which group IDs are valid. You may call this function at any time, even 10 minutes later,
  /// and it will correctly determine the results.
  static bool IsTaskGroupFinished(ezTaskGroupID Group);

  /// \brief Call this function once at the end of a frame. It will ensure that all tasks for 'this frame' get finished properly.
  ///
  /// Calling this function is crucial for several reasons. It is the central function to execute 'main thread' tasks.
  /// Otherwise these tasks might never get executed. It also changes the priority of all 'next frame' tasks to 'this frame',
  /// so that those tasks are guaranteed to get finished when 'FinishFrameTasks' is called the next time.
  ///
  /// Finally this function executes tasks with the priority 'SomeFrameMainThread' as long as the target frame time is not exceeded.
  /// You can configure this with \a fSmoothFrameMS, which defines how long (in milliseconds) the frame is allowed to be. As long
  /// as that time is not exceeded, additional 'main thread' tasks will be executed.
  /// If the frame time spikes for a few frames, no such tasks will be executed, to prevent making it worse. However, if the frame
  /// time stays high over a longer period, 'FinishFrameTasks' will execute 'SomeFrameMainThread' tasks every once in a while,
  /// to guarantee some progress.
  static void FinishFrameTasks(double fSmoothFrameMS = 1000.0 / 40.0 /* 40 FPS -> 25 ms */);

  /// \brief This function will block until the given task has finished.
  ///
  /// This function guarantees that pTask will get executed eventually. Instead of idly waiting for the task to finish,
  /// it actively executes tasks. This is especially important when it is run on the main thread, as there might be
  /// tasks in the system that can only be executed on the main thread, which are also dependencies for pTask.
  /// In such a case these tasks must be executed properly, or otherwise one would wait forever.
  ///
  /// \note With the current implementation there is no guarantee that this function terminates as soon as the given
  /// task is finished, since this thread might be busy running some other task.
  /// This function is only meant to synchronize code, it is not meant to be efficient and should therefore not be
  /// called in regular scenarios.
  static void WaitForTask(ezTask* pTask);

  /// \brief Blocks until all tasks in the given group have finished. Similar to 'WaitForTask'.
  static void WaitForGroup(ezTaskGroupID Group);

  /// \brief This function will try to remove the given task from the work queue, to prevent it from being executed.
  ///
  /// The function will return EZ_SUCCESS, if the task could be removed and thus its execution could be prevented.
  /// It will also return EZ_SUCCESS, if the task was already finished and nothing needed to be done.
  /// Tasks that are removed without execution will still be marked as 'finished' and dependent tasks will be scheduled.
  ///
  /// EZ_FAILURE is returned, if the task had already been started and thus could not be prevented from running.
  ///
  /// In case of failure, \a bWaitForIt determines whether 'WaitForTask' is called (with all its consequences), 
  /// or whether the function will return immediately.
  ///
  /// The cancel flag is set on the task, such that tasks that support canceling might terminate earlier.
  /// However, there is no guarantee how long it takes for already running tasks to actually finish.
  /// Therefore when bWaitForIt is true, this function might block for a very long time.
  /// It is advised to implement tasks that need to be canceled regularly (e.g. path searches for units that might die)
  /// in a way that allows for quick canceling.
  static ezResult CancelTask(ezTask* pTask, ezOnTaskRunning::Enum OnTaskRunning = ezOnTaskRunning::WaitTillFinished);

  /// \brief Cancels all the tasks in the given group.
  ///
  /// EZ_SUCCESS is returned, if all tasks were already finished or could be removed without waiting for any of them.
  /// EZ_FAILURE is returned, if at least one task was being processed by another thread and could not be removed without waiting.
  /// If bWaitForIt is false, the function cancels all tasks, but returns without blocking, even if not all tasks have been finished.
  /// If bWaitForIt is true, the function returns only after it is guaranteed that all tasks are properly terminated.
  static ezResult CancelGroup(ezTaskGroupID Group, ezOnTaskRunning::Enum OnTaskRunning = ezOnTaskRunning::WaitTillFinished);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TaskSystem2);
  friend class ezTaskWorkerThread;

  static void Startup();
  static void Shutdown();

private:
  struct TaskData
  {
    ezTask* m_pTask;
    ezTaskGroup* m_pBelongsToGroup;
  };

  // The arrays of all the active worker threads.
  static ezDynamicArray<ezTaskWorkerThread*, ezStaticAllocatorWrapper> s_WorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // Checks that group are valid for configuration.
  static void DebugCheckTaskGroup(ezTaskGroupID Group);

  // Takes all the tasks in the given group and schedules them for execution, by inserting them into the proper task lists.
  static void ScheduleGroupTasks(ezTaskGroup* pGroup);

  // Called whenever a task has been finished/canceled. Makes sure that groups are marked as finished when all tasks are done.
  static void TaskHasFinished(ezTask* pTask, ezTaskGroup* pGroup);

  // Is called whenever a dependency of pGroup has finished. Once all dependencies are finished, the group's tasks will get scheduled.
  static void DependencyHasFinished(ezTaskGroup* pGroup);

  // Shuts down all worker threads. Does NOT finish the remaining tasks. Does not clear them either, though.
  static void StopWorkerThreads();

  // Searches for a task of priority between \a FirstPriority and \a LastPriority (inclusive).
  static TaskData GetNextTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, ezTask* pPrioritizeThis = NULL);

  // Executes some task of priority between \a FirstPriority and \a LastPriority (inclusive). Returns true, if any such task was available.
  static bool ExecuteTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, ezTask* pPrioritizeThis = NULL);

  // Ensures all 'main thread' tasks for this frame are finished ('ThisFrameMainThread').
  static void FinishMainThreadTasks();

  // Moves all 'next frame' tasks into the 'this frame' queues.
  static void ReprioritizeFrameTasks();

  // Executes up to uiSomeFrameTasks tasks of priority 'SomeFrameMainThread', as long as the last duration between frames is no longer than fSmoothFrameMS.
  static void ExecuteSomeFrameTasks(ezUInt32 uiSomeFrameTasks, double fSmoothFrameMS);

private:
  // *** Internal Data ***

  // One mutex to rule them all.
  static ezMutex s_TaskSystemMutex;

  // The deque can grow without relocating existing data, therefore the ezTaskGroupID's can store pointers directly to the data
  static ezDeque<ezTaskGroup, ezStaticAllocatorWrapper> s_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  static ezList<TaskData, ezStaticAllocatorWrapper> s_Tasks[ezTaskPriority::ENUM_COUNT];

  // Thread signals to wake up a worker thread of the proper type, whenever new work becomes available.
  static ezThreadSignal s_TasksAvailableSignal[ezWorkerThreadType::ENUM_COUNT];

  // some data for profiling
  static ezProfilingId s_ProfileWaitForTask;
  static ezProfilingId s_ProfileWaitForGroup;
  static ezProfilingId s_ProfileCancelTask;
  static ezProfilingId s_ProfileCancelGroup;
  static ezProfilingId s_ProfileMainThreadTasks;
  static ezProfilingId s_ProfileSomeFrameTasks;
};


