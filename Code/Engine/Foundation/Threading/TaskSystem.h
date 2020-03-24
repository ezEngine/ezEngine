#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Threading/Implementation/Task.h>
#include <Foundation/Threading/Mutex.h>

class ezDGMLGraph;

/// \brief This system allows to automatically distribute tasks onto a number of worker threads.
///
/// By deriving from ezTask you can create your own task types. These can be executed through this task system.
/// You can run a single task using the 'StartSingleTask' function. For more complex setups, it is possible
/// to create groups of tasks, which can have interdependencies. This should be used to group all
/// tasks that belong to one system and need to be done before another system runs. For example you could group
/// all tasks to update particle systems, and then have another group for all tasks to update sound, which depends
/// on the first group, such that sound is only updated after all particle systems are done with.
///
/// Although it is possible to wait for tasks or to cancel them, it is generally advised to try to minimize their use.
/// Tasks that might need to be canceled regularly (e.g. path searches) should be implemented in a way that they
/// are aware of being canceled and will stop their work prematurely, instead of running through to the end.
///
/// Note that it is crucial to call 'FinishFrameTasks' once per frame, otherwise tasks that need to be executed on the
/// main thread are never executed.
class EZ_FOUNDATION_DLL ezTaskSystem
{
public:
  /// \brief Sets the number of threads to use for the different task categories.
  ///
  /// \a uiShortTasks and \a uiLongTasks must be at least 1 and should not exceed the number of available CPU cores.
  /// There will always be exactly one additional thread for file access tasks (ezTaskPriority::FileAccess).
  ///
  /// If \a uiShortTasks or \a uiLongTasks is smaller than 1, a default number of threads will be used for that type of work.
  /// This number of threads depends on the number of available CPU cores.
  /// If SetWorkThreadCount is never called, at all, the first time any task is started the number of worker threads is set to
  /// this default configuration.
  /// Unless you have a good idea how to set up the number of worker threads to make good use of the available cores,
  /// it is a good idea to just use the default settings.
  static void SetWorkerThreadCount(ezInt8 iShortTasks = -1, ezInt8 iLongTasks = -1); // [tested]

  /// \brief Returns the maximum number of threads that should work on the given type of task at the same time.
  static ezUInt32 GetWorkerThreadCount(ezWorkerThreadType::Enum type) { return s_MaxWorkerThreadsToUse[type]; }

  /// \brief Returns the number of threads that have been allocated to potentially work on the given type of task.
  ///
  /// CAREFUL! This is not the number of threads that will be active at the same time. Use GetWorkerThreadCount() for that.
  /// This is the maximum number of threads that may jump in, if too many threads are blocked. This number will change dynamically
  /// at runtime to prevent deadlocks and it can grow very, very large.
  static ezUInt32 GetNumAllocatedWorkerThreads(ezWorkerThreadType::Enum type) { return s_iNumWorkerThreads[type]; }

  /// \brief A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task
  /// has been put.
  static ezTaskGroupID StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority); // [tested]

  /// \brief A helper function to insert a single task into the system and start it right away. Returns ID of the Group into which the task
  /// has been put. This overload allows to additionally specify a single dependency.
  static ezTaskGroupID StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority, ezTaskGroupID Dependency); // [tested]

  /// \brief Class allowing to change certain parameters of a parallel for invocation.
  struct EZ_FOUNDATION_DLL ParallelForParams
  {
    ParallelForParams() {} // do not remove, needed for Clang

    /// The minimum number of items that must be processed by a task instance.
    /// If the overall number of tasks lies below this value, all work will be executed purely serially
    /// without involving any tasks at all.
    ezUInt32 uiBinSize = 1;
    /// Indicates how many tasks per thread may be spawned at most by a ParallelFor invocation.
    /// Higher numbers give the scheduler more leeway to balance work across available threads.
    /// Generally, if all task items are expected to take basically the same amount of time,
    /// low numbers (usually 1) are recommended, while higher numbers (initially test with 2 or 3)
    /// might yield better results for workloads where task items may take vastly different amounts
    /// of time, such that scheduling in a balanced fashion becomes more difficult.
    ezUInt32 uiMaxTasksPerThread = 2;

    /// Returns the multiplicity to use for the given task. If 0 is returned,
    /// serial execution is to be performed.
    ezUInt32 DetermineMultiplicity(ezUInt32 uiNumTaskItems);
    /// Returns the number of task items to work on per invocation (multiplicity).
    /// This is aligned with the multiplicity, i.e., multiplicity * bin_size >= # task items.
    ezUInt32 DetermineItemsPerInvocation(ezUInt32 uiNumTaskItems, ezUInt32 uiMultiplicity);
  };

  using ParallelForIndexedFunction = ezDelegate<void(ezUInt32, ezUInt32), 48>;

  template <typename ElemType>
  using ParallelForFunction = ezDelegate<void(ezUInt32, ezArrayPtr<ElemType>), 48>;

  /// A helper function to process task items in a parallel fashion by having per-worker index ranges generated.
  static void ParallelForIndexed(ezUInt32 uiStartIndex, ezUInt32 uiNumItems, ParallelForIndexedFunction taskCallback, const char* taskName = nullptr, ParallelForParams config = ParallelForParams());

  /// A helper function to process task items in a parallel fashion by generating per-worker sub-ranges
  /// from an initial item array pointer.
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](ezArrayPtr<ElemType> taskItemSlice) { });
  template <typename ElemType, typename Callback>
  static void ParallelFor(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName = nullptr, ParallelForParams params = ParallelForParams());
  /// A helper function to process task items in a parallel fashion and one-by-one (without global index).
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](ElemType taskItem) { });
  ///   - ParallelFor(taskItems, [](ElemType& taskItem) { });
  ///   - ParallelFor(taskItems, [](const ElemType& taskItem) { });
  template <typename ElemType, typename Callback>
  static void ParallelForSingle(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName = nullptr, ParallelForParams params = ParallelForParams());
  /// A helper function to process task items in a parallel fashion and one-by-one (with global index).
  /// Given an array pointer 'taskItems' with elements of type ElemType, the following invocations are possible:
  ///   - ParallelFor(taskItems, [](ezUInt32 globalTaskItemIndex, ElemType taskItem) { });
  ///   - ParallelFor(taskItems, [](ezUInt32 globalTaskItemIndex, ElemType& taskItem) { });
  ///   - ParallelFor(taskItems, [](ezUInt32 globalTaskItemIndex, const ElemType& taskItem) { });
  template <typename ElemType, typename Callback>
  static void ParallelForSingleIndex(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName = nullptr, ParallelForParams params = ParallelForParams());

  /// \brief Creates a new task group for one-time use. Groups need to be recreated every time a task is supposed to be inserted into the
  /// system.
  ///
  /// All tasks that are added to this group will be run with the same given \a Priority.
  /// Once all tasks in the group are finished and thus the group is finished, an optional \a Callback can be executed.
  static ezTaskGroupID CreateTaskGroup(ezTaskPriority::Enum Priority, OnTaskGroupFinishedCallback callback = OnTaskGroupFinishedCallback()); // [tested]

  /// \brief Adds a task to the given task group. The group must not yet have been started.
  static void AddTaskToGroup(ezTaskGroupID Group, ezTask* pTask); // [tested]

  /// \brief Adds a dependency on another group to \a Group. This means \a Group will not be execute before \a DependsOn has finished.
  ///
  /// \note Be careful with dependencies and task priorities. A task that has to execute 'this frame' should never depend on a task
  /// that needs only finish 'next frame', this might introduce very long and unnecessary waits.
  /// A task that has priority 'this frame' or 'next frame' will actually not be executed in 'this frame' or 'next frame' until all
  /// its dependencies are fulfilled. So you might add a long running task and a short task which depends on it, but the system will
  /// not block at the end of the frame, to wait for the long running task (to finish the short task thereafter), as that short task
  /// won't get scheduled for execution, at all, until all its dependencies are actually finished.
  static void AddTaskGroupDependency(ezTaskGroupID Group, ezTaskGroupID DependsOn); // [tested]

  /// \brief Same as AddTaskGroupDependency() but batches multiple dependency additions
  static void AddTaskGroupDependencyBatch(ezArrayPtr<const ezTaskGroupDependency> batch);

  /// \brief Starts the task group. After this no further modifications on the group (new tasks or dependencies) are allowed.
  static void StartTaskGroup(ezTaskGroupID Group); // [tested]

  /// \brief Same as StartTaskGroup() but batches multiple actions
  static void StartTaskGroupBatch(ezArrayPtr<const ezTaskGroupID> batch);

  /// \brief Returns whether the given \a Group id refers to a task group that has been finished already.
  ///
  /// There is no time frame in which group IDs are valid. You may call this function at any time, even 10 minutes later,
  /// and it will correctly determine the results.
  static bool IsTaskGroupFinished(ezTaskGroupID Group); // [tested]

  /// \brief Sets the target frame time that is supposed to not be exceeded.
  ///
  /// \see FinishFrameTasks() for more details.
  static void SetTargetFrameTime(double fSmoothFrameMS = 1000.0 / 40.0 /* 40 FPS -> 25 ms */);

  /// \brief Call this function once at the end of a frame. It will ensure that all tasks for 'this frame' get finished properly.
  ///
  /// Calling this function is crucial for several reasons. It is the central function to execute 'main thread' tasks.
  /// Otherwise these tasks might never get executed. It also changes the priority of all 'next frame' tasks to 'this frame',
  /// so that those tasks are guaranteed to get finished when 'FinishFrameTasks' is called the next time.
  ///
  /// Finally this function executes tasks with the priority 'SomeFrameMainThread' as long as the target frame time is not exceeded.
  /// You can configure this with SetTargetFrameTime(), which defines how long (in milliseconds) the frame is allowed to be. As long
  /// as that time is not exceeded, additional 'SomeFrameMainThread' tasks will be executed.
  /// If the frame time spikes for a few frames, no such tasks will be executed, to prevent making it worse. However, if the frame
  /// time stays high over a longer period, 'FinishFrameTasks' will execute 'SomeFrameMainThread' tasks every once in a while,
  /// to guarantee some progress.
  ///
  /// \note After this function returns all tasks of priority 'ThisFrameMainThread' are finished. All tasks of priority
  /// 'EarlyThisFrame' up to 'LateThisFrame' are either finished or currently running on some thread, so they will be finished soon.
  /// There is however no guarantee that they are indeed all finished, as that would introduce unnecessary stalls.
  static void FinishFrameTasks(); // [tested]

  /// \brief Blocks until all tasks in the given group have finished.
  ///
  /// If you need to wait for some other task to finish, this should always be the preferred method to do so.
  /// WaitForGroup will put the current thread to sleep and use thread signals to only wake it up again once the group is indeed
  /// finished. This is the most efficient way to wait for a task.
  static void WaitForGroup(ezTaskGroupID Group); // [tested]

  /// \brief Blocks the current thread until the given delegate returns true.
  ///
  /// If possible, prefer to use WaitForGroup() to wait for some task to finish, as that is the most efficient way.
  /// If not possible, prefer to use WaitForCondition() instead of rolling your own busy-loop for polling some state.
  /// WaitForCondition() will NOT put the current thread to sleep, but instead keep polling the delegate. However, in between,
  /// it will try to execute other tasks and if there are no tasks that it could take on, it will wake up another worker thread
  /// thus guaranteeing, that there are enough unblocked threads in the system to do all the work.
  static void WaitForCondition(ezDelegate<bool()> condition);

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
  static ezResult CancelTask(ezTask* pTask, ezOnTaskRunning::Enum OnTaskRunning = ezOnTaskRunning::WaitTillFinished); // [tested]

  /// \brief Cancels all the tasks in the given group.
  ///
  /// EZ_SUCCESS is returned, if all tasks were already finished or could be removed without waiting for any of them.
  /// EZ_FAILURE is returned, if at least one task was being processed by another thread and could not be removed without waiting.
  /// If bWaitForIt is false, the function cancels all tasks, but returns without blocking, even if not all tasks have been finished.
  /// If bWaitForIt is true, the function returns only after it is guaranteed that all tasks are properly terminated.
  static ezResult CancelGroup(ezTaskGroupID Group, ezOnTaskRunning::Enum OnTaskRunning = ezOnTaskRunning::WaitTillFinished); // [tested]

  /// \brief Returns the (thread local) type of tasks that would be executed on this thread
  static ezWorkerThreadType::Enum GetCurrentThreadWorkerType();

  /// \brief Returns the utilization (0.0 to 1.0) of the given thread. Note: This will only be valid, if FinishFrameTasks() is called once
  /// per frame.
  ///
  /// Also optionally returns the number of tasks that were finished during the last frame.
  static double GetThreadUtilization(ezWorkerThreadType::Enum Type, ezUInt32 uiThreadIndex, ezUInt32* pNumTasksExecuted = nullptr);

  /// \brief Writes the internal state of the ezTaskSystem as a DGML graph.
  static void WriteStateSnapshotToDGML(ezDGMLGraph& graph);

  /// \brief Convenience function to write the task graph snapshot to a file. If no path is given, the file is written to
  /// ":appdata/TaskGraphs/__date__.dgml"
  static void WriteStateSnapshotToFile(const char* szPath = nullptr);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, TaskSystem);
  friend class ezTaskWorkerThread;

  static void Startup();
  static void Shutdown();

private:
  struct TaskData
  {
    ezTask* m_pTask = nullptr;
    ezTaskGroup* m_pBelongsToGroup = nullptr;
    ezUInt32 m_uiInvocation = 0;
  };

  // The arrays of all the active worker threads.
  static ezDynamicArray<ezTaskWorkerThread*> s_WorkerThreads[ezWorkerThreadType::ENUM_COUNT];
  // the number of allocated (non-null) worker threads in s_WorkerThreads
  static ezAtomicInteger32 s_iNumWorkerThreads[ezWorkerThreadType::ENUM_COUNT];
  // the maximum number of worker threads that should be non-idle (and not blocked) at any time
  static ezUInt32 s_MaxWorkerThreadsToUse[ezWorkerThreadType::ENUM_COUNT];

  // Takes all the tasks in the given group and schedules them for execution, by inserting them into the proper task lists.
  static void ScheduleGroupTasks(ezTaskGroup* pGroup, bool bHighPriority);

  // Called whenever a task has been finished/canceled. Makes sure that groups are marked as finished when all tasks are done.
  static void TaskHasFinished(ezTask* pTask, ezTaskGroup* pGroup);

  // Is called whenever a dependency of pGroup has finished. Once all dependencies are finished, the group's tasks will get scheduled.
  static void DependencyHasFinished(ezTaskGroup* pGroup);

  // Shuts down all worker threads. Does NOT finish the remaining tasks. Does not clear them either, though.
  static void StopWorkerThreads();

  // Searches for a task of priority between \a FirstPriority and \a LastPriority (inclusive).
  static TaskData GetNextTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicBool* pIsIdleNow);

  // Executes some task of priority between \a FirstPriority and \a LastPriority (inclusive). Returns true, if any such task was available.
  static bool ExecuteTask(ezTaskPriority::Enum FirstPriority, ezTaskPriority::Enum LastPriority, bool bOnlyTasksThatNeverWait, const ezTaskGroupID& WaitingForGroup, ezAtomicBool* pIsIdleNow);

  // Moves all 'next frame' tasks into the 'this frame' queues.
  static void ReprioritizeFrameTasks();

  // Executes up to uiSomeFrameTasks tasks of priority 'SomeFrameMainThread', as long as the last duration between frames is no longer than
  // fSmoothFrameMS.
  static void ExecuteSomeFrameTasks(ezUInt32 uiSomeFrameTasks, double fSmoothFrameMS);

  // Figures out the range of tasks that this thread may execute when it has free cycles to help out.
  // Uses a thread local variable to know whether this is the main thread / loading thread / long running thread and thus also decides
  // whether the thread is allowed to fall back to 'default' work (short tasks).
  static void DetermineTasksToExecuteOnThread(ezTaskPriority::Enum& out_FirstPriority, ezTaskPriority::Enum& out_LastPriority);

  template <typename ElemType>
  static void ParallelForInternal(ezArrayPtr<ElemType> taskItems, ParallelForFunction<ElemType> taskCallback, const char* taskName, ParallelForParams config);

  /// \brief Helps executing tasks that are suitable for the calling thread. Returns true if a task was found and executed.
  static bool HelpExecutingTasks(const ezTaskGroupID& WaitingForGroup);

  static void AllocateThreads(ezWorkerThreadType::Enum type, ezUInt32 uiAddThreads);

  /// \brief Calculates how many worker threads may get activated. Number can be negative, when we are already above budget.
  static ezInt32 CalcActivatableThreads(ezWorkerThreadType::Enum type);

  static void WakeUpThreads(ezWorkerThreadType::Enum type, ezUInt32 uiNumThreads);

  /// \brief If the given thread is currently idle, it will be woken up and EZ_SUCCESS is returned. Otherwise EZ_FAILURE is returned and no thread is woken up.
  static ezResult WakeUpThreadIfIdle(ezWorkerThreadType::Enum type, ezUInt32 threadIdx);

private:
  // *** Internal Data ***

  // One mutex to rule them all.
  static ezMutex s_TaskSystemMutex;

  // The deque can grow without relocating existing data, therefore the ezTaskGroupID's can store pointers directly to the data
  static ezDeque<ezTaskGroup> s_TaskGroups;

  // The lists of all scheduled tasks, for each priority.
  static ezList<TaskData> s_Tasks[ezTaskPriority::ENUM_COUNT];

  // only for debugging
  static ezAtomicInteger32 s_IdleWorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // need to know how many threads are non-idle but blocked
  static ezAtomicInteger32 s_BlockedWorkerThreads[ezWorkerThreadType::ENUM_COUNT];

  // The target frame time used by FinishFrameTasks()
  static double s_fSmoothFrameMS;
};

#include <Foundation/Threading/Implementation/ParallelFor_inl.h>
