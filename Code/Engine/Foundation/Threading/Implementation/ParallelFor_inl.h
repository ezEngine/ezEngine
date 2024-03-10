#pragma once

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

template <typename ElemType>
class ArrayPtrTask final : public ezTask
{
public:
  ArrayPtrTask(ezArrayPtr<ElemType> payload, ezParallelForFunction<ElemType> taskCallback, ezUInt32 uiItemsPerInvocation)
    : m_Payload(payload)
    , m_uiItemsPerInvocation(uiItemsPerInvocation)
    , m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(0, m_Payload);
  }

  void ExecuteWithMultiplicity(ezUInt32 uiInvocation) const override
  {
    const ezUInt32 uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;

    const ezUInt32 uiRemainingItems = uiSliceStartIndex > m_Payload.GetCount() ? 0 : m_Payload.GetCount() - uiSliceStartIndex;
    const ezUInt32 uiSliceItemCount = ezMath::Min(m_uiItemsPerInvocation, uiRemainingItems);

    if (uiSliceItemCount > 0)
    {
      // Run through the calculated slice.
      auto taskItemSlice = m_Payload.GetSubArray(uiSliceStartIndex, uiSliceItemCount);
      m_TaskCallback(uiSliceStartIndex, taskItemSlice);
    }
  }

private:
  ezArrayPtr<ElemType> m_Payload;
  ezUInt32 m_uiItemsPerInvocation;
  ezParallelForFunction<ElemType> m_TaskCallback;
};

template <typename ElemType>
void ezTaskSystem::ParallelForInternal(ezArrayPtr<ElemType> taskItems, ezParallelForFunction<ElemType> taskCallback, const char* taskName, const ezParallelForParams& params)
{
  if (taskItems.GetCount() <= params.m_uiBinSize)
  {
    ArrayPtrTask<ElemType> arrayPtrTask(taskItems, std::move(taskCallback), taskItems.GetCount());
    arrayPtrTask.ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    EZ_PROFILE_SCOPE(arrayPtrTask.m_sTaskName);
    arrayPtrTask.Execute();
  }
  else
  {
    ezUInt32 uiMultiplicity;
    ezUInt64 uiItemsPerInvocation;
    params.DetermineThreading(taskItems.GetCount(), uiMultiplicity, uiItemsPerInvocation);

    ezAllocator* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : ezFoundation::GetDefaultAllocator();

    ezSharedPtr<ArrayPtrTask<ElemType>> pArrayPtrTask = EZ_NEW(pAllocator, ArrayPtrTask<ElemType>, taskItems, std::move(taskCallback), static_cast<ezUInt32>(uiItemsPerInvocation));
    pArrayPtrTask->ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    pArrayPtrTask->SetMultiplicity(uiMultiplicity);
    ezTaskGroupID taskGroupId = ezTaskSystem::StartSingleTask(pArrayPtrTask, ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::WaitForGroup(taskGroupId);
  }
}

template <typename ElemType, typename Callback>
void ezTaskSystem::ParallelFor(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const ezParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](
                           ezUInt32 /*uiBaseIndex*/, ezArrayPtr<ElemType> taskSlice)
  { taskCallback(taskSlice); };

  ParallelForInternal<ElemType>(
    taskItems, ezParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void ezTaskSystem::ParallelForSingle(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const ezParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](ezUInt32 /*uiBaseIndex*/, ezArrayPtr<ElemType> taskSlice)
  {
    // Handing in by non-const& allows to use callbacks with (non-)const& as well as value parameters.
    for (ElemType& taskItem : taskSlice)
    {
      taskCallback(taskItem);
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, ezParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void ezTaskSystem::ParallelForSingleIndex(
  ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const ezParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](ezUInt32 uiBaseIndex, ezArrayPtr<ElemType> taskSlice)
  {
    for (ezUInt32 uiIndex = 0; uiIndex < taskSlice.GetCount(); ++uiIndex)
    {
      // Handing in by dereferenced pointer allows to use callbacks with (non-)const& as well as value parameters.
      taskCallback(uiBaseIndex + uiIndex, *(taskSlice.GetPtr() + uiIndex));
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, ezParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}
