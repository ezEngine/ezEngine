#pragma once

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

template <typename ElemType>
class ArrayPtrTask : public ezTask
{
public:
  ArrayPtrTask(ezArrayPtr<ElemType> payload, ezTaskSystem::ParallelForFunction<ElemType> taskCallback, ezUInt32 uiItemsPerInvocation)
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
  ezTaskSystem::ParallelForFunction<ElemType> m_TaskCallback;
};

template <typename ElemType>
void ezTaskSystem::ParallelForInternal(ezArrayPtr<ElemType> taskItems, ParallelForFunction<ElemType> taskCallback, const char* taskName, ParallelForParams config)
{
  const ezUInt32 uiMultiplicity = config.DetermineMultiplicity(taskItems.GetCount());
  const ezUInt32 uiItemsPerInvocation = config.DetermineItemsPerInvocation(taskItems.GetCount(), uiMultiplicity);

  ArrayPtrTask<ElemType> arrayPtrTask(taskItems, std::move(taskCallback), uiItemsPerInvocation);
  arrayPtrTask.SetTaskName(taskName ? taskName : "Generic ArrayPtr Task");
  
  if (uiMultiplicity == 0)
  {
    EZ_PROFILE_SCOPE(arrayPtrTask.m_sTaskName);
    arrayPtrTask.Execute();
  }
  else
  {
    arrayPtrTask.SetMultiplicity(uiMultiplicity);

    ezTaskGroupID taskGroupId = ezTaskSystem::StartSingleTask(&arrayPtrTask, ezTaskPriority::EarlyThisFrame);
    ezTaskSystem::WaitForGroup(taskGroupId);
  }
}

template<typename ElemType, typename Callback>
void ezTaskSystem::ParallelFor(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName, ParallelForParams params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](ezUInt32 /*uiBaseIndex*/, ezArrayPtr<ElemType> taskSlice)
  {
    taskCallback(taskSlice);
  };

  ParallelForInternal<ElemType>(taskItems, ParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), taskName, params);
}

template <typename ElemType, typename Callback>
void ezTaskSystem::ParallelForSingle(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName, ParallelForParams params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](ezUInt32 /*uiBaseIndex*/, ezArrayPtr<ElemType> taskSlice)
  {
    // Handing in by non-const& allows to use callbacks with (non-)const& as well as value parameters.
    for (ElemType& taskItem : taskSlice)
    {
      taskCallback(taskItem);
    }
  };

  ParallelForInternal<ElemType>(taskItems, ParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), taskName, params);
}

template <typename ElemType, typename Callback>
void ezTaskSystem::ParallelForSingleIndex(ezArrayPtr<ElemType> taskItems, Callback taskCallback, const char* taskName, ParallelForParams params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](ezUInt32 uiBaseIndex, ezArrayPtr<ElemType> taskSlice)
  {
    for (ezUInt32 uiIndex = 0; uiIndex < taskSlice.GetCount(); ++uiIndex)
    {
      // Handing in by dereferenced pointer allows to use callbacks with (non-)const& as well as value parameters.
      taskCallback(uiBaseIndex + uiIndex, *(taskSlice.GetPtr() + uiIndex));
    }
  };

  ParallelForInternal<ElemType>(taskItems, ParallelForFunction<ElemType>(std::move(wrappedCallback), ezFrameAllocator::GetCurrentAllocator()), taskName, params);
}

