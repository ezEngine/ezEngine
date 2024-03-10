#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Implementation/TaskGroup.h>
#include <Foundation/Threading/Implementation/TaskSystemState.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Utilities/DGMLWriter.h>

const char* ezWorkerThreadType::GetThreadTypeName(ezWorkerThreadType::Enum threadType)
{
  switch (threadType)
  {
    case ezWorkerThreadType::ShortTasks:
      return "Short Task";

    case ezWorkerThreadType::LongTasks:
      return "Long Task";

    case ezWorkerThreadType::FileAccess:
      return "File Access";

    default:
      EZ_REPORT_FAILURE("Invalid Thread Type");
      return "unknown";
  }
}

void ezTaskSystem::WriteStateSnapshotToDGML(ezDGMLGraph& ref_graph)
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

  const ezDGMLGraph::PropertyId startedByUserId = ref_graph.AddPropertyType("StartByUser");
  const ezDGMLGraph::PropertyId activeDepsId = ref_graph.AddPropertyType("ActiveDependencies");
  const ezDGMLGraph::PropertyId scheduledId = ref_graph.AddPropertyType("Scheduled");
  const ezDGMLGraph::PropertyId finishedId = ref_graph.AddPropertyType("Finished");
  const ezDGMLGraph::PropertyId multiplicityId = ref_graph.AddPropertyType("Multiplicity");
  const ezDGMLGraph::PropertyId remainingRunsId = ref_graph.AddPropertyType("RemainingRuns");
  const ezDGMLGraph::PropertyId priorityId = ref_graph.AddPropertyType("GroupPriority");

  const char* szTaskPriorityNames[ezTaskPriority::ENUM_COUNT] = {};
  szTaskPriorityNames[ezTaskPriority::EarlyThisFrame] = "EarlyThisFrame";
  szTaskPriorityNames[ezTaskPriority::ThisFrame] = "ThisFrame";
  szTaskPriorityNames[ezTaskPriority::LateThisFrame] = "LateThisFrame";
  szTaskPriorityNames[ezTaskPriority::EarlyNextFrame] = "EarlyNextFrame";
  szTaskPriorityNames[ezTaskPriority::NextFrame] = "NextFrame";
  szTaskPriorityNames[ezTaskPriority::LateNextFrame] = "LateNextFrame";
  szTaskPriorityNames[ezTaskPriority::In2Frames] = "In 2 Frames";
  szTaskPriorityNames[ezTaskPriority::In3Frames] = "In 3 Frames";
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

  for (ezUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const ezTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    title.SetFormat("Group {}", g);

    const ezDGMLGraph::NodeId taskGroupId = ref_graph.AddGroup(title, ezDGMLGraph::GroupType::Expanded, &taskGroupND);
    groupNodeIds[&tg] = taskGroupId;

    ref_graph.AddNodeProperty(taskGroupId, startedByUserId, tg.m_bStartedByUser ? "true" : "false");
    ref_graph.AddNodeProperty(taskGroupId, priorityId, szTaskPriorityNames[tg.m_Priority]);
    ref_graph.AddNodeProperty(taskGroupId, activeDepsId, ezFmt("{}", tg.m_iNumActiveDependencies));

    for (ezUInt32 t = 0; t < tg.m_Tasks.GetCount(); ++t)
    {
      const ezTask& task = *tg.m_Tasks[t];
      const ezDGMLGraph::NodeId taskNodeId = ref_graph.AddNode(task.m_sTaskName, &taskNodeND);

      ref_graph.AddNodeToGroup(taskNodeId, taskGroupId);

      ref_graph.AddNodeProperty(taskNodeId, scheduledId, task.m_bTaskIsScheduled ? "true" : "false");
      ref_graph.AddNodeProperty(taskNodeId, finishedId, task.IsTaskFinished() ? "true" : "false");

      tmp.SetFormat("{}", task.GetMultiplicity());
      ref_graph.AddNodeProperty(taskNodeId, multiplicityId, tmp);

      tmp.SetFormat("{}", task.m_iRemainingRuns);
      ref_graph.AddNodeProperty(taskNodeId, remainingRunsId, tmp);
    }
  }

  for (ezUInt32 g = 0; g < s_pState->m_TaskGroups.GetCount(); ++g)
  {
    const ezTaskGroup& tg = s_pState->m_TaskGroups[g];

    if (!tg.m_bInUse)
      continue;

    const ezDGMLGraph::NodeId ownNodeId = groupNodeIds[&tg];

    for (const ezTaskGroupID& dependsOn : tg.m_DependsOnGroups)
    {
      ezDGMLGraph::NodeId otherNodeId;

      // filter out already fulfilled dependencies
      if (dependsOn.m_pTaskGroup->m_uiGroupCounter != dependsOn.m_uiGroupCounter)
        continue;

      // filter out already fulfilled dependencies
      if (!groupNodeIds.TryGetValue(dependsOn.m_pTaskGroup, otherNodeId))
        continue;

      EZ_ASSERT_DEBUG(otherNodeId != ownNodeId, "");

      ref_graph.AddConnection(otherNodeId, ownNodeId);
    }
  }
}

void ezTaskSystem::WriteStateSnapshotToFile(const char* szPath /*= nullptr*/)
{
  ezStringBuilder sPath = szPath;

  if (sPath.IsEmpty())
  {
    sPath = ":appdata/TaskGraphs/";

    const ezDateTime dt = ezDateTime::MakeFromTimestamp(ezTimestamp::CurrentTimestamp());

    sPath.AppendFormat("{0}-{1}-{2}_{3}-{4}-{5}-{6}", dt.GetYear(), ezArgU(dt.GetMonth(), 2, true), ezArgU(dt.GetDay(), 2, true), ezArgU(dt.GetHour(), 2, true), ezArgU(dt.GetMinute(), 2, true), ezArgU(dt.GetSecond(), 2, true), ezArgU(dt.GetMicroseconds() / 1000, 3, true));

    sPath.ChangeFileExtension("dgml");
  }

  ezDGMLGraph graph;
  ezTaskSystem::WriteStateSnapshotToDGML(graph);

  ezDGMLGraphWriter::WriteGraphToFile(sPath, graph).IgnoreResult();

  ezStringBuilder absPath;
  ezFileSystem::ResolvePath(sPath, &absPath, nullptr).IgnoreResult();
  ezLog::Info("Task graph snapshot saved to '{}'", absPath);
}
