#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

ezExtractedRenderData::ezExtractedRenderData() = default;

void ezExtractedRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetFinalSortingKey(category, m_Camera);
}

void ezExtractedRenderData::AddFrameData(const ezRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void ezExtractedRenderData::SortAndBatch()
{
  EZ_PROFILE_SCOPE("ezExtractedRenderData::SortAndBatch");

  struct RenderDataComparer
  {
    EZ_FORCE_INLINE bool Less(const ezRenderDataBatch::SortableRenderData& a, const ezRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey != b.m_uiSortingKey)
      {
        return a.m_uiSortingKey < b.m_uiSortingKey;
      }

      return a.m_pRenderData->m_hOwner < b.m_pRenderData->m_hOwner;
    }
  };

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    EZ_PROFILE_SCOPE("SortCategory");

    auto& data = dataPerCategory.m_SortableRenderData;

    // Sort
    data.Sort(RenderDataComparer());

    // Find batches
    const ezRenderData* pCurrentBatchRenderData = data[0].m_pRenderData;
    const ezRTTI* pCurrentBatchType = pCurrentBatchRenderData->GetDynamicRTTI();
    ezUInt32 uiCurrentBatchStartIndex = 0;

    for (ezUInt32 i = 1; i < data.GetCount(); ++i)
    {
      auto pRenderData = data[i].m_pRenderData;

      if (pRenderData->GetDynamicRTTI() != pCurrentBatchType || pRenderData->CanBatch(*pCurrentBatchRenderData) == false)
      {
        dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], i - uiCurrentBatchStartIndex);

        pCurrentBatchRenderData = pRenderData;
        pCurrentBatchType = pRenderData->GetDynamicRTTI();
        uiCurrentBatchStartIndex = i;
      }
    }

    dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
  }
}

void ezExtractedRenderData::Clear()
{
  for (auto& dataPerCategory : m_DataPerCategory)
  {
    dataPerCategory.m_Batches.Clear();
    dataPerCategory.m_SortableRenderData.Clear();
  }

  m_FrameData.Clear();
}

ezRenderDataBatchList ezExtractedRenderData::GetRenderDataBatchesWithCategory(ezRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    ezRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;

    return list;
  }

  return ezRenderDataBatchList();
}

ezArrayPtr<const ezRenderDataBatch::SortableRenderData> ezExtractedRenderData::GetRawRenderDataWithCategory(ezRenderData::Category category) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    return m_DataPerCategory[category.m_uiValue].m_SortableRenderData;
  }

  return {};
}

const ezRenderData* ezExtractedRenderData::GetFrameData(const ezRTTI* pRtti) const
{
  for (auto pData : m_FrameData)
  {
    if (pData->IsInstanceOf(pRtti))
    {
      return pData;
    }
  }

  return nullptr;
}
