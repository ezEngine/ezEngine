#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/BatchedRenderData.h>

#include <Foundation/Profiling/Profiling.h>

static ezProfilingId s_SortingProfilingId = ezProfilingSystem::CreateId("SortAndBatch");

ezBatchedRenderData::ezBatchedRenderData()
{
}

void ezBatchedRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category)
{
  if (category >= m_DataPerCategory.GetCount())
  {
    m_DataPerCategory.SetCount(category + 1);
  }

  m_DataPerCategory[category].m_SortedRenderData.PushBack(pRenderData);
}

void ezBatchedRenderData::SortAndBatch()
{
  EZ_PROFILE(s_SortingProfilingId);

  struct RenderDataComparer
  {
    EZ_FORCE_INLINE bool Less(const SortableRenderData& a, const SortableRenderData& b) const
    {
      return a.m_uiSortingKey < b.m_uiSortingKey;
    }
  };

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    if (dataPerCategory.m_SortedRenderData.IsEmpty())
      continue;

    auto& sortedRenderData = dataPerCategory.m_SortedRenderData;

    m_TempDataForSorting.Clear();

    // Copy to temp array for sorting
    for (auto pRenderData : sortedRenderData)
    {
      auto& sortableRenderData = m_TempDataForSorting.ExpandAndGetRef();
      sortableRenderData.m_pRenderData = pRenderData;
      sortableRenderData.m_uiSortingKey = pRenderData->m_uiBatchId; // TODO: implement proper sorting
    }

    // Sort
    m_TempDataForSorting.Sort(RenderDataComparer());

    // Copy back
    for (ezUInt32 i = 0; i < m_TempDataForSorting.GetCount(); ++i)
    {
      sortedRenderData[i] = m_TempDataForSorting[i].m_pRenderData;
    }   

    // Find batches
    ezUInt32 uiCurrentBatchId = sortedRenderData[0]->m_uiBatchId;
    ezUInt32 uiCurrentBatchStartIndex = 0;
    const ezRTTI* pCurrentBatchType = sortedRenderData[0]->GetDynamicRTTI();

    for (ezUInt32 i = 1; i < sortedRenderData.GetCount(); ++i)
    {
      auto pRenderData = sortedRenderData[i];

      if (pRenderData->m_uiBatchId != uiCurrentBatchId || pRenderData->GetDynamicRTTI() != pCurrentBatchType)
      {
        ezArrayPtr<const ezRenderData*> batch(&sortedRenderData[uiCurrentBatchStartIndex], i - uiCurrentBatchStartIndex);
        dataPerCategory.m_Batches.PushBack(batch);

        uiCurrentBatchId = pRenderData->m_uiBatchId;
        uiCurrentBatchStartIndex = i;
        pCurrentBatchType = pRenderData->GetDynamicRTTI();
      }
    }

    ezArrayPtr<const ezRenderData*> batch(&sortedRenderData[uiCurrentBatchStartIndex], sortedRenderData.GetCount() - uiCurrentBatchStartIndex);
    dataPerCategory.m_Batches.PushBack(batch);
  }
}

void ezBatchedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortedRenderData.Clear();
  }

  // TODO: intelligent compact
}
