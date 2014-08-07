#pragma once

#include <Foundation/Threading/Thread.h>
#include <Foundation/Containers/List.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Time/Time.h>


class ezTaskGroup;
class ezTask;

/// \brief Describes the priority with which to execute a task.
///
/// For tasks that you start this frame and that need to finish within the same frame,
/// use 'EarlyThisFrame', 'ThisFrame' or 'LateThisFrame'.\n
/// However you should generally not rely on starting tasks in the same frame in which they need to finish.\n
/// Instead prefer to start a task, whose result you need in the next frame.\n
/// For those tasks, use 'EarlyNextFrame', 'NextFrame' and 'LateNextFrame'.\n
/// Once 'ezTaskSystem::FinishFrameTasks' is called, all those tasks will be moved into the 'XYZThisFrame' categories.\n
/// For tasks that run over a longer period (e.g. path searches, procedural data creation), use 'LongRunning'.
/// Only use 'LongRunningHighPriority' for tasks that occur rarely, otherwise 'LongRunning' tasks might not get processed, at all.\n
/// For tasks that need to access files, prefer to use 'FileAccess', this way all file accesses get executed sequentially.\n
/// Use 'FileAccessHighPriority' to get very important file accesses done sooner. For example writing out a save-game should finish quickly.\n
/// For tasks that need to execute on the main thread (e.g. uploading GPU resources) use 'ThisFrameMainThread' or 'SomeFrameMainThread'
/// depending on how urgent it is. 'SomeFrameMainThread' tasks might get delayed for quite a while, depending on the system load.
/// For tasks that need to do several things (e.g. reading from a file AND uploading something on the main thread), split them up into
/// several tasks that depend on each other (or just let the first task start the next step once it is finished).
/// There is no guarantee WHEN (within a frame) main thread tasks get executed. Most of them will get executed upon a 'FinishFrameTasks' call.
/// However they are also run whenever the main thread needs to wait or cancel some other task and has nothing else to do.
/// So tasks that get executed on the main thread should never assume a certain state of other systems.
struct ezTaskPriority
{
  enum Enum
  {
    EarlyThisFrame,             ///< Highest priority, guaranteed to get finished in this frame.
    ThisFrame,                  ///< Medium priority, guaranteed to get finished in this frame.
    LateThisFrame,              ///< Low priority, guaranteed to get finished in this frame.
    EarlyNextFrame,             ///< Highest priority in next frame, guaranteed to get finished this frame or the next.
    NextFrame,                  ///< Medium priority in next frame, guaranteed to get finished this frame or the next.
    LateNextFrame,              ///< Low priority in next frame, guaranteed to get finished this frame or the next.
    LongRunningHighPriority,    ///< Tasks that might take a while, but should be preferred over 'LongRunning' tasks. Use this priority only rarely, otherwise 'LongRunning' tasks might never get executed.
    LongRunning,                ///< Use this priority for tasks that might run for a while.
    FileAccessHighPriority,     ///< For tasks that require file access (e.g. resource loading). They run on one dedicated thread, such that file accesses are done sequentially and never in parallel.
    FileAccess,                 ///< For tasks that require file access (e.g. resource loading). They run on one dedicated thread, such that file accesses are done sequentially and never in parallel.
    ThisFrameMainThread,        ///< Tasks that need to be executed this frame, but in the main thread. This is mostly intended for resource creation.
    SomeFrameMainThread,        ///< Tasks that have no hard deadline but need to be executed in the main thread. This is mostly intended for resource creation.

    ENUM_COUNT
  };
};

/// \brief Enum that describes what to do when waiting for or canceling tasks, that have already started execution.
struct ezOnTaskRunning
{
  enum Enum
  {
    WaitTillFinished,
    ReturnWithoutBlocking
  };
};

/// \internal Enum that lists the different task worker thread types.
struct ezWorkerThreadType
{
  enum Enum
  {
    ShortTasks,
    LongTasks,
    FileAccess,
    ENUM_COUNT
  };
};

/// \internal Internal task worker thread class.
class ezTaskWorkerThread : public ezThread
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezTaskWorkerThread);

private:
  friend class ezTaskSystem;

  /// \brief Tells the worker thread what tasks to execute and which thread index it has.
  ezTaskWorkerThread(ezWorkerThreadType::Enum ThreadType, ezUInt32 iThreadNumber);

  // Whether the thread is supposed to continue running.
  volatile bool m_bActive;

  // Which types of tasks this thread should work on.
  ezWorkerThreadType::Enum m_WorkerType;

  // For display purposes.
  ezUInt8 m_uiWorkerThreadNumber;

  virtual ezUInt32 Run() override;

  // Computes the thread utilization by dividing the thread active time by the time that has passed since the last update.
  void ComputeThreadUtilization(ezTime TimePassed);

  // The thread keeps track of how much time it spends executing tasks. This function retrieves that time and resets it to zero.
  ezTime GetAndResetThreadActiveTime();

  bool m_bExecutingTask;
  ezTime m_StartedWorking;
  ezTime m_ThreadActiveTime;
  double m_ThreadUtilization;
  ezAtomicInteger32 m_iTasksExecutionCounter;
  ezUInt32 m_uiNumTasksExecuted;
};

/// \brief Given out by ezTaskSystem::CreateTaskGroup to identify a task group.
class EZ_FOUNDATION_DLL ezTaskGroupID
{
public:
  ezTaskGroupID();

private:
  friend class ezTaskSystem;

  // the counter is used to determine whether this group id references the 'same' group, as m_pTaskGroup.
  // if m_pTaskGroup->m_uiGroupCounter is different to this->m_uiGroupCounter, then the group ID is not valid anymore.
  volatile ezUInt32 m_uiGroupCounter;

  // points to the actual task group object
  ezTaskGroup* m_pTaskGroup;
};

/// \internal Should have been a nested struct in ezTaskSystem, but that does not work with forward declarations.
class ezTaskGroup
{
public:
  /// \brief The function type to use when one wants to get informed when a task group has been finished.
  typedef void(*OnTaskGroupFinished)(void* pPassThrough);

private:
  friend class ezTaskSystem;

  ezTaskGroup();

  bool m_bInUse;
  bool m_bIsActive;
  ezUInt16 m_uiGroupCounter;
  ezHybridArray<ezTask*, 16> m_Tasks;
  ezHybridArray<ezTaskGroupID, 4> m_DependsOn;
  ezHybridArray<ezTaskGroupID, 8> m_OthersDependingOnMe;
  ezAtomicInteger32 m_iActiveDependencies;
  ezAtomicInteger32 m_iRemainingTasks;
  ezTaskPriority::Enum m_Priority;
  OnTaskGroupFinished m_OnFinishedCallback;
  void* m_pCallbackPassThrough;
};

