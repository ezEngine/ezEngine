#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/TaskSystem.h>

/// \brief This is a helper class that splits up task items via index ranges.
template<typename IndexType, typename Callback>
class IndexedTask final : public ezTask
{
public:
  IndexedTask(IndexType uiStartIndex, IndexType uiNumItems, Callback taskCallback, IndexType uiItemsPerInvocation)
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
    const IndexType uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;
    const IndexType uiSliceEndIndex = ezMath::Min(uiSliceStartIndex + m_uiItemsPerInvocation, m_uiStartIndex + m_uiNumItems);

    // Run through the calculated slice, the end index is exclusive, i.e., should not be handled by this instance.
    m_TaskCallback(uiSliceStartIndex, uiSliceEndIndex);
  }

private:
  IndexType m_uiStartIndex;
  IndexType m_uiNumItems;
  IndexType m_uiItemsPerInvocation;
  Callback m_TaskCallback;
};

template <typename IndexType, typename Callback>
void ParallelForIndexedInternal(IndexType uiStartIndex, IndexType uiNumItems, const Callback& taskCallback, const char* taskName, const ezParallelForParams& params)
{
  typedef IndexedTask<IndexType, Callback> Task;

  if (!taskName)
  {
    taskName = "Generic Indexed Task";
  }

  const ezUInt32 uiMultiplicity = params.DetermineMultiplicity(uiNumItems);
  const IndexType uiItemsPerInvocation = params.DetermineItemsPerInvocation(uiNumItems, uiMultiplicity);

  if (uiMultiplicity == 0)
  {
    Task indexedTask(uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    indexedTask.ConfigureTask(taskName, ezTaskNesting::Never);

    EZ_PROFILE_SCOPE(taskName);
    indexedTask.Execute();
  }
  else
  {
    ezAllocatorBase* pAllocator = (params.pTaskAllocator != nullptr) ? params.pTaskAllocator : ezFoundation::GetDefaultAllocator();

    ezSharedPtr<Task> pIndexedTask = EZ_NEW(pAllocator, Task, uiStartIndex, uiNumItems, std::move(taskCallback), uiItemsPerInvocation);
    pIndexedTask->ConfigureTask(taskName, ezTaskNesting::Never);

    pIndexedTask->SetMultiplicity(uiMultiplicity);
    ezTaskGroupID taskGroupId = ezTaskSystem::StartSingleTask(pIndexedTask, ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::WaitForGroup(taskGroupId);
  }
}

ezUInt32 ezParallelForParams::DetermineMultiplicity(ezUInt64 uiNumTaskItems) const
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
    const ezUInt64 numThreads = (uiNumTaskItems + uiBinSize - 1) / uiBinSize;
    EZ_ASSERT_DEV(numThreads <= uiNumWorkers, "");
    EZ_ASSERT_DEV(numThreads < (1ull << 32), "");
    return (ezUInt32)numThreads;
  }
  // Needing at most #_workers * threading_factor threads.
  else if (uiNumTaskItems <= uiSliceSize * uiMaxTasksPerThread)
  {
    const ezUInt64 uiNumSlices = (uiNumTaskItems + uiSliceSize - 1) / uiSliceSize;
    EZ_ASSERT_DEV(uiNumSlices <= uiMaxTasksPerThread, "");
    const ezUInt64 result = uiNumSlices * uiNumWorkers;
    EZ_ASSERT_DEV(result < (1ull << 32), "");
    return (ezUInt32)result;
  }
  // Needing more than #_workers * threading_factor threads --> clamp to that number.
  else
  {
    const ezUInt64 result = uiNumWorkers * uiMaxTasksPerThread;
    EZ_ASSERT_DEV(result < (1ull << 32), "");
    return (ezUInt32)result;
  }
}

ezUInt64 ezParallelForParams::DetermineItemsPerInvocation(ezUInt64 uiNumTaskItems, ezUInt32 uiMultiplicity) const
{
  if (uiMultiplicity == 0)
  {
    return uiNumTaskItems;
  }

  const ezUInt64 uiItemsPerInvocation = (uiNumTaskItems + uiMultiplicity - 1) / uiMultiplicity;
  return uiItemsPerInvocation;
}

ezUInt32 ezParallelForParams::DetermineItemsPerInvocation(ezUInt32 uiNumTaskItems, ezUInt32 uiMultiplicity) const
{
  const ezUInt64 result = DetermineItemsPerInvocation(ezUInt64(uiNumTaskItems), uiMultiplicity);
  EZ_ASSERT_DEV(result < (1ull << 32), "");
  return (ezUInt32)result;
}

void ezTaskSystem::ParallelForIndexed(
  ezUInt32 uiStartIndex, ezUInt32 uiNumItems, ezParallelForIndexedFunction32 taskCallback, const char* taskName, const ezParallelForParams& params)
{
  ParallelForIndexedInternal<ezUInt32, ezParallelForIndexedFunction32>(uiStartIndex, uiNumItems, taskCallback, taskName, params);
}

void ezTaskSystem::ParallelForIndexed(
  ezUInt64 uiStartIndex, ezUInt64 uiNumItems, ezParallelForIndexedFunction64 taskCallback, const char* taskName, const ezParallelForParams& params)
{
  ParallelForIndexedInternal<ezUInt64, ezParallelForIndexedFunction64>(uiStartIndex, uiNumItems, taskCallback, taskName, params);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ParallelFor);
