#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

ezMutex ezTaskSystem::s_TaskSystemMutex;
double ezTaskSystem::s_fSmoothFrameMS = 1000.0 / 40.0; // => 25 ms
ezThreadSignal ezTaskSystem::s_TasksAvailableSignal[ezWorkerThreadType::ENUM_COUNT];
ezDynamicArray<ezTaskWorkerThread*> ezTaskSystem::s_WorkerThreads[ezWorkerThreadType::ENUM_COUNT];
ezDeque<ezTaskGroup> ezTaskSystem::s_TaskGroups;
ezList<ezTaskSystem::TaskData> ezTaskSystem::s_Tasks[ezTaskPriority::ENUM_COUNT];

thread_local ezWorkerThreadType::Enum g_ThreadTaskType = ezWorkerThreadType::Unknown;

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, TaskSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ThreadUtils",
    "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezTaskSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezTaskSystem::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

void ezTaskSystem::Startup()
{
  g_ThreadTaskType = ezWorkerThreadType::MainThread;
}

void ezTaskSystem::Shutdown()
{
  StopWorkerThreads();
}

ezTaskGroupID ezTaskSystem::StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority, ezTaskGroupID Dependency)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority);
  AddTaskGroupDependency(Group, Dependency);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

ezTaskGroupID ezTaskSystem::StartSingleTask(ezTask* pTask, ezTaskPriority::Enum Priority)
{
  ezTaskGroupID Group = CreateTaskGroup(Priority);
  AddTaskToGroup(Group, pTask);
  StartTaskGroup(Group);
  return Group;
}

void ezTaskSystem::FinishMainThreadTasks()
{
  EZ_PROFILE_SCOPE("ThisFrameMainThreadTasks");

  bool bGotStuffToDo = true;

  while (bGotStuffToDo)
  {
    bGotStuffToDo = false;

    // Prefer to work on main-thread tasks
    if (ExecuteTask(ezTaskPriority::ThisFrameMainThread, ezTaskPriority::ThisFrameMainThread))
    {
      bGotStuffToDo = true;
      continue;
    }

    // if there are none, help out with the other tasks for this frame
    if (ExecuteTask(ezTaskPriority::EarlyThisFrame, ezTaskPriority::LateThisFrame))
    {
      bGotStuffToDo = true;
      continue;
    }
  }
}

void ezTaskSystem::ReprioritizeFrameTasks()
{
  // There should usually be no 'this frame tasks' left at this time
  // however, while we waited to enter the lock, such tasks might have appeared
  // In this case we move them into the highest-priority 'this frame' queue, to ensure they will be executed asap
  for (ezUInt32 i = (ezUInt32)ezTaskPriority::ThisFrame; i <= (ezUInt32)ezTaskPriority::LateThisFrame; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'this frame' tasks into the 'early this frame' queue
    while (it.IsValid())
    {
      s_Tasks[ezTaskPriority::EarlyThisFrame].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::EarlyNextFrame; i <= (ezUInt32)ezTaskPriority::LateNextFrame; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'next frame' tasks into the 'this frame' queues
    while (it.IsValid())
    {
      s_Tasks[i - 3].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }

  for (ezUInt32 i = (ezUInt32)ezTaskPriority::In2Frames; i <= (ezUInt32)ezTaskPriority::In9Frames; ++i)
  {
    auto it = s_Tasks[i].GetIterator();

    // move all 'in N frames' tasks into the 'in N-1 frames' queues
    // moves 'In2Frames' into 'LateNextFrame'
    while (it.IsValid())
    {
      s_Tasks[i - 1].PushBack(*it);

      ++it;
    }

    // remove the tasks from their current queue
    s_Tasks[i].Clear();
  }
}

void ezTaskSystem::ExecuteSomeFrameTasks(ezUInt32 uiSomeFrameTasks, double fSmoothFrameMS)
{
  if (uiSomeFrameTasks == 0)
    return;

  EZ_PROFILE_SCOPE("SomeFrameMainThreadTasks");

  // 'SomeFrameMainThread' tasks are usually used to upload resources that have been loaded in the background
  // they do not need to be executed right away, but the earlier, the better

  // as long as the frame time is short enough, execute tasks that need to be done on the main thread
  // on fast machines that means that these tasks are finished as soon as possible and users will see the results quickly

  // if the frame time spikes, we can skip this a few times, to try to prevent further slow downs
  // however in such instances, the 'frame time threshold' will increase and thus the chance that we skip this entirely becomes lower over
  // time that guarantees some progress, even if the frame rate is constantly low

  static double s_fFrameTimeThreshold = fSmoothFrameMS;

  static ezTime s_LastFrame; // initializes to zero -> very large frame time difference at first

  ezTime CurTime = ezTime::Now();
  ezTime LastTime = s_LastFrame;
  s_LastFrame = CurTime;

  // as long as we have a smooth frame rate, execute as many of these tasks, as possible
  while ((uiSomeFrameTasks > 0) && ((CurTime - LastTime).GetMilliseconds() < fSmoothFrameMS))
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_fFrameTimeThreshold = fSmoothFrameMS;

    if (!ExecuteTask(ezTaskPriority::SomeFrameMainThread, ezTaskPriority::SomeFrameMainThread))
      return; // nothing left to do

    CurTime = ezTime::Now();
    --uiSomeFrameTasks;
  }

  // nothing left to do
  if (uiSomeFrameTasks == 0)
    return;

  if ((CurTime - LastTime).GetMilliseconds() < s_fFrameTimeThreshold)
  {
    // we execute one of these tasks, so reset the frame time threshold
    s_fFrameTimeThreshold = fSmoothFrameMS;

    ExecuteTask(ezTaskPriority::SomeFrameMainThread, ezTaskPriority::SomeFrameMainThread);
  }
  else
  {
    // increase the threshold by 5 ms
    // this means that when the frame rate is too low, we can ignore these tasks for a few frames
    // and thus prevent decreasing the frame rate even further
    // however we increase the time threshold, at which we skip this, further and further
    // therefore at some point we will execute at least one such task, no matter how low the frame rate is
    // this guarantees at least some progress with these tasks
    s_fFrameTimeThreshold += 5.0;

    //  25 ms -> 40 FPS
    //  30 ms -> 33 FPS
    //  35 ms -> 28 FPS
    //  40 ms -> 25 FPS
    //  45 ms -> 22 FPS
    //  50 ms -> 20 FPS
    //  55 ms -> 18 FPS
    //  60 ms -> 16 FPS
    //  65 ms -> 15 FPS
    //  70 ms -> 14 FPS
    //  75 ms -> 13 FPS
  }
}

void ezTaskSystem::DetermineTasksToExecuteOnThread(
  ezTaskPriority::Enum& out_FirstPriority, ezTaskPriority::Enum& out_LastPriority, bool& out_bAllowDefaultWork)
{
  // this specifies whether WaitForTask may fall back to processing standard tasks, when there is no more specific work available
  // in some cases we absolutely want to avoid that, since it can produce deadlocks
  // E.g. on the loading thread, if we are in the process of loading something and then we have to wait for something else,
  // we must not start that work on the loading thread, because once THAT task runs into something where it has to wait for something
  // to be loaded, we have a circular dependency on the thread itself and thus a deadlock
  out_bAllowDefaultWork = true;

  out_FirstPriority = ezTaskPriority::EarlyThisFrame;
  out_LastPriority = ezTaskPriority::In9Frames;

  switch (g_ThreadTaskType)
  {
    case ezWorkerThreadType::MainThread:
    {
      // if this is the main thread, we need to execute the main-thread tasks
      // otherwise a dependency on which Group is waiting, might not get fulfilled
      out_FirstPriority = ezTaskPriority::ThisFrameMainThread;
      out_LastPriority = ezTaskPriority::SomeFrameMainThread;
      out_bAllowDefaultWork = false;
      break;
    }

    case ezWorkerThreadType::FileAccess:
    {
      out_FirstPriority = ezTaskPriority::FileAccessHighPriority;
      out_LastPriority = ezTaskPriority::FileAccess;
      out_bAllowDefaultWork = false;
      break;
    }

    case ezWorkerThreadType::LongTasks:
    {
      // if this is a long-running thread, we need to execute the long-running tasks
      // otherwise a dependency on which Group is waiting, might not get fulfilled
      out_FirstPriority = ezTaskPriority::LongRunningHighPriority;
      out_LastPriority = ezTaskPriority::LongRunning;
      break;
    }

    case ezWorkerThreadType::Unknown:
    {
      // probably a thread not launched through ez
      [[fallthrough]];
    }

    case ezWorkerThreadType::ShortTasks:
    {
      break;
    }

    default:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
    }
  }
}

void ezTaskSystem::SetTargetFrameTime(double fSmoothFrameMS)
{
  s_fSmoothFrameMS = fSmoothFrameMS;
}

void ezTaskSystem::FinishFrameTasks()
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "This function must be executed on the main thread.");

  FinishMainThreadTasks();

  ezUInt32 uiSomeFrameTasks = 0;

  // now all the important tasks for this frame should be finished
  // so now we re-prioritize the tasks for the next frame
  {
    EZ_LOCK(s_TaskSystemMutex);

    // get this info once, it won't shrink (but might grow) while we are outside the lock
    uiSomeFrameTasks = s_Tasks[ezTaskPriority::SomeFrameMainThread].GetCount();

    ReprioritizeFrameTasks();
  }

  ExecuteSomeFrameTasks(uiSomeFrameTasks, s_fSmoothFrameMS);

  // Update the thread utilization
  {
    const ezTime tNow = ezTime::Now();
    static ezTime s_LastFrameUpdate = tNow;
    const ezTime tDiff = tNow - s_LastFrameUpdate;

    // prevent division by zero (inside ComputeThreadUtilization)
    if (tDiff > ezTime::Seconds(0.0))
    {
      s_LastFrameUpdate = tNow;

      for (ezUInt32 type = 0; type < ezWorkerThreadType::ENUM_COUNT; ++type)
      {
        for (ezUInt32 t = 0; t < s_WorkerThreads[type].GetCount(); ++t)
        {
          s_WorkerThreads[type][t]->ComputeThreadUtilization(tDiff);
        }
      }
    }
  }
}

void ezTaskSystem::WriteStateSnapshotToDGML(ezDGMLGraph& graph)
{
  EZ_LOCK(s_TaskSystemMutex);

  ezHashTable<const ezTaskGroup*, ezDGMLGraph::NodeId> groupNodeIds;

  ezStringBuilder title, tmp;

  ezDGMLGraph::NodeDesc taskGroupND;
  taskGroupND.m_Color = ezColor::CornflowerBlue;
  taskGroupND.m_Shape = ezDGMLGraph::NodeShape::Rectangle;

  ezDGMLGraph::NodeDesc taskNodeND;
  taskNodeND.m_Color = ezColor::OrangeRed;
  taskNodeND.m_Shape = ezDGMLGraph::NodeShape::RoundedRectangle;

  const ezDGMLGraph::PropertyId startedByUserId = graph.AddPropertyType("StartByUser");
  const ezDGMLGraph::PropertyId activeDepsId = graph.AddPropertyType("ActiveDependencies");
  const ezDGMLGraph::PropertyId scheduledId = graph.AddPropertyType("Scheduled");
  const ezDGMLGraph::PropertyId finishedId = graph.AddPropertyType("Finished");
  const ezDGMLGraph::PropertyId multiplicityId = graph.AddPropertyType("Multiplicity");
  const ezDGMLGraph::PropertyId remainingRunsId = graph.AddPropertyType("RemainingRuns");
  const ezDGMLGraph::PropertyId priorityId = graph.AddPropertyType("GroupPriority");

  const char* szTaskPriorityNames[ezTaskPriority::ENUM_COUNT] = {};
  szTaskPriorityNames[ezTaskPriority::EarlyThisFrame] = "EarlyThisFrame";
  szTaskPriorityNames[ezTaskPriority::ThisFrame] = "ThisFrame";
  szTaskPriorityNames[ezTaskPriority::LateThisFrame] = "LateThisFrame";
  szTaskPriorityNames[ezTaskPriority::EarlyNextFrame] = "EarlyNextFrame";
  szTaskPriorityNames[ezTaskPriority::NextFrame] = "NextFrame";
  szTaskPriorityNames[ezTaskPriority::LateNextFrame] = "LateNextFrame";
  szTaskPriorityNames[ezTaskPriority::In2Frames] = "In 2 Frames";
  szTaskPriorityNames[ezTaskPriority::In2Frames] = "In 3 Frames";
  szTaskPriorityNames[ezTaskPriority::In4Frames] = "In 4 Frames";
  szTaskPriorityNames[ezTaskPriority::In5Frames] = "In 5 Frames";
  szTaskPriorityNames[ezTaskPriority::In6Frames] = "In 6 Frames";
  szTaskPriorityNames[ezTaskPriority::In7Frames] = "In 7 Frames";
  szTaskPriorityNames[ezTaskPriority::In8Frames] = "In 8 Frames";
  szTaskPriorityNames[ezTaskPriority::In9Frames] = "In 9 Frames";
  szTaskPriorityNames[ezTaskPriority::LongRunningHighPriority] = "LongRunningHighPriority";
  szTaskPriorityNames[ezTaskPriority::LongRunning] = "LongRunning";
  szTaskPriorityNames[ezTaskPriority::FileAccessHighPriority] = "FileAccessHighPriority";
  szTaskPriorityNames[ezTaskPriority::FileAccess] = "FileAccess";
  szTaskPriorityNames[ezTaskPriority::ThisFrameMainThread] = "ThisFrameMainThread";
  szTaskPriorityNames[ezTaskPriority::SomeFrameMainThread] = "SomeFrameMainThread";

  for (ezUInt32 g = 0; g < s_TaskGroups.GetCount(); ++g)
  {
    const ezTaskGroup& tg = s_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    title.Format("Group {}", g);

    const ezDGMLGraph::NodeId taskGroupId = graph.AddGroup(title, ezDGMLGraph::GroupType::Expanded, &taskGroupND);
    groupNodeIds[&tg] = taskGroupId;

    graph.AddNodeProperty(taskGroupId, startedByUserId, tg.m_bStartedByUser ? "true" : "false");
    graph.AddNodeProperty(taskGroupId, priorityId, szTaskPriorityNames[tg.m_Priority]);
    graph.AddNodeProperty(taskGroupId, activeDepsId, ezFmt("{}", tg.m_iActiveDependencies));

    for (ezUInt32 t = 0; t < tg.m_Tasks.GetCount(); ++t)
    {
      const ezTask& task = *tg.m_Tasks[t];
      const ezDGMLGraph::NodeId taskNodeId = graph.AddNode(task.m_sTaskName, &taskNodeND);

      graph.AddNodeToGroup(taskNodeId, taskGroupId);

      graph.AddNodeProperty(taskNodeId, scheduledId, task.m_bTaskIsScheduled ? "true" : "false");
      graph.AddNodeProperty(taskNodeId, finishedId, task.IsTaskFinished() ? "true" : "false");

      tmp.Format("{}", task.GetMultiplicity());
      graph.AddNodeProperty(taskNodeId, multiplicityId, tmp);

      tmp.Format("{}", task.m_iRemainingRuns);
      graph.AddNodeProperty(taskNodeId, remainingRunsId, tmp);
    }
  }

  for (ezUInt32 g = 0; g < s_TaskGroups.GetCount(); ++g)
  {
    const ezTaskGroup& tg = s_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    const ezDGMLGraph::NodeId ownNodeId = groupNodeIds[&tg];

    for (const ezTaskGroupID& dependsOn : tg.m_DependsOn)
    {
      ezDGMLGraph::NodeId otherNodeId;

      // filter out already fulfilled dependencies
      if (dependsOn.m_pTaskGroup->m_uiGroupCounter != dependsOn.m_uiGroupCounter)
        continue;

      // filter out already fulfilled dependencies
      if (!groupNodeIds.TryGetValue(dependsOn.m_pTaskGroup, otherNodeId))
        continue;

      EZ_ASSERT_DEBUG(otherNodeId != ownNodeId, "");

      graph.AddConnection(otherNodeId, ownNodeId);
    }
  }
}

void ezTaskSystem::WriteStateSnapshotToFile(const char* szPath /*= nullptr*/)
{
  ezStringBuilder sPath = szPath;

  if (sPath.IsEmpty())
  {
    sPath = ":appdata/TaskGraphs/";

    const ezDateTime dt = ezTimestamp::CurrentTimestamp();

    sPath.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true),
      ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true),
      ezArgU(dt.GetMicroseconds() / 1000, 3, true));

    sPath.ChangeFileExtension("dgml");
  }

  ezDGMLGraph graph;
  ezTaskSystem::WriteStateSnapshotToDGML(graph);

  ezDGMLGraphWriter::WriteGraphToFile(sPath, graph);

  ezStringBuilder absPath;
  ezFileSystem::ResolvePath(sPath, &absPath, nullptr);
  ezLog::Info("Task graph snapshot saved to '{}'", absPath);
}

ezWorkerThreadType::Enum ezTaskSystem::GetCurrentThreadWorkerType()
{
  return g_ThreadTaskType;
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_TaskSystem);
