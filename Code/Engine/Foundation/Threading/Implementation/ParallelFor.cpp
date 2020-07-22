#include <FoundationPCH.h>

#include <Foundation/Threading/TaskSystem.h>

/// \brief This is a helper class that splits up task items via index ranges.
class IndexedTask final : public ezTask
{
public:
  IndexedTask(ezUInt32 uiStartIndex, ezUInt32 uiNumItems, ezParallelForIndexedFunction taskCallback, ezUInt32 uiItemsPerInvocation)
    : m_uiStartIndex(uiStartIndex)
    , m_uiNumItems(uiNumItems)
    , m_uiItemsPerInvocation(uiItemsPerInvocation)
    , m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(m_uiStartIndex, m_uiStartIndex + m_uiNumItems);
  }

  void ExecuteWithMultiplicity(ezUInt32 uiInvocation) const override
  {
    const ezUInt32 uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;
    const ezUInt32 uiSliceEndIndex = ezMath::Min(uiSliceStartIndex + m_uiItemsPerInvocation, m_uiStartIndex + m_uiNumItems);

    // Run through the calculated slice, the end index is exclusive, i.e., should not be handled by this instance.
    m_TaskCallback(uiSliceStartIndex, uiSliceEndIndex);
  }

private:
  ezUInt32 m_uiStartIndex;
  ezUInt32 m_uiNumItems;
  ezUInt32 m_uiItemsPerInvocation;
  ezParallelForIndexedFunction m_TaskCallback;
};

ezUInt32 ezParallelForParams::DetermineMultiplicity(ezUInt32 uiNumTaskItems) const
{
  // If we have not exceeded the threading threshold we will indicate to use serial execution.
  if (uiNumTaskItems < uiBinSize)
  {
    return 0;
  }

  const ezUInt32 uiNumWorkers = ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks);
  // The slice size gives the number of items that can be processed when giving exactly uiBinSize
  // task items to each worker.
  const ezUInt32 uiSliceSize = uiNumWorkers * uiBinSize;

  // Needing at most #_workers threads.
  if (uiNumTaskItems <= uiSliceSize)
  {
    // Fill up each thread with at most uiBinSize task items.
    const ezUInt32 numThreads = (uiNumTaskItems + uiBinSize - 1) / uiBinSize;
    EZ_ASSERT_DEV(numThreads <= uiNumWorkers, "");
    return numThreads;
  }
  // Needing at most #_workers * threading_factor threads.
  else if (uiNumTaskItems <= uiSliceSize * uiMaxTasksPerThread)
  {
    const ezUInt32 uiNumSlices = (uiNumTaskItems + uiSliceSize - 1) / uiSliceSize;
    EZ_ASSERT_DEV(uiNumSlices <= uiMaxTasksPerThread, "");
    return uiNumSlices * uiNumWorkers;
  }
  // Needing more than #_workers * threading_factor threads --> clamp to that number.
  else
  {
    return uiNumWorkers * uiMaxTasksPerThread;
  }
}

ezUInt32 ezParallelForParams::DetermineItemsPerInvocation(ezUInt32 uiNumTaskItems, ezUInt32 uiMultiplicity) const
{
  if (uiMultiplicity == 0)
  {
    return uiNumTaskItems;
  }

  const ezUInt32 uiItemsPerInvocation = (uiNumTaskItems + uiMultiplicity - 1) / uiMultiplicity;
  return uiItemsPerInvocation;
}

void ezTaskSystem::ParallelForIndexed(
  ezUInt32 uiStartIndex, ezUInt32 uiNumItems, ezParallelForIndexedFunction taskCallback, const char* taskName, const ezParallelForParams& params)
{
  const ezUInt32 uiMultiplicity = params.DetermineMultiplicity(uiNumItems);
  const ezUInt32 uiItemsPerInvocation = params.DetermineItemsPerInvocation(uiNumItems, uiMultiplicity);


  if (uiMultiplicity == 0)
  {
    IndexedTask indexedTask(uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    indexedTask.ConfigureTask(taskName ? taskName : "Generic Indexed Task", ezTaskNesting::Never);

    EZ_PROFILE_SCOPE(indexedTask.m_sTaskName);
    indexedTask.Execute();
  }
  else
  {
    ezAllocatorBase* pAllocator = (params.pTaskAllocator != nullptr) ? params.pTaskAllocator : ezFoundation::GetDefaultAllocator();

    ezSharedPtr<IndexedTask> pIndexedTask = EZ_NEW(pAllocator, IndexedTask, uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    pIndexedTask->ConfigureTask(taskName ? taskName : "Generic Indexed Task", ezTaskNesting::Never);

    pIndexedTask->SetMultiplicity(uiMultiplicity);
    ezTaskGroupID taskGroupId = ezTaskSystem::StartSingleTask(pIndexedTask, ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::WaitForGroup(taskGroupId);
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ParallelFor);
