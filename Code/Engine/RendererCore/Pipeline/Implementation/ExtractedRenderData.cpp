#include <RendererCorePCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

ezExtractedRenderData::ezExtractedRenderData() {}

void ezExtractedRenderData::AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category)
{
  m_DataPerCategory.EnsureCount(category.m_uiValue + 1);

  auto& sortableRenderData = m_DataPerCategory[category.m_uiValue].m_SortableRenderData.ExpandAndGetRef();
  sortableRenderData.m_pRenderData = pRenderData;
  sortableRenderData.m_uiSortingKey = pRenderData->GetCategorySortingKey(category, m_Camera);
}

void ezExtractedRenderData::AddFrameData(const ezRenderData* pFrameData)
{
  m_FrameData.PushBack(pFrameData);
}

void ezExtractedRenderData::SortAndBatch()
{
  EZ_PROFILE_SCOPE("SortAndBatch");

  struct RenderDataComparer
  {
    EZ_FORCE_INLINE bool Less(const ezRenderDataBatch::SortableRenderData& a, const ezRenderDataBatch::SortableRenderData& b) const
    {
      if (a.m_uiSortingKey == b.m_uiSortingKey)
      {
        return a.m_pRenderData->m_uiBatchId < b.m_pRenderData->m_uiBatchId;
      }

      return a.m_uiSortingKey < b.m_uiSortingKey;
    }
  };

  for (auto& dataPerCategory : m_DataPerCategory)
  {
    if (dataPerCategory.m_SortableRenderData.IsEmpty())
      continue;

    auto& data = dataPerCategory.m_SortableRenderData;

    // Sort
    data.Sort(RenderDataComparer());

    // Find batches
    ezUInt32 uiCurrentBatchId = data[0].m_pRenderData->m_uiBatchId;
    ezUInt32 uiCurrentBatchStartIndex = 0;
    const ezRTTI* pCurrentBatchType = data[0].m_pRenderData->GetDynamicRTTI();

    for (ezUInt32 i = 1; i < data.GetCount(); ++i)
    {
      auto pRenderData = data[i].m_pRenderData;

      if (pRenderData->m_uiBatchId != uiCurrentBatchId || pRenderData->GetDynamicRTTI() != pCurrentBatchType)
      {
        dataPerCategory.m_Batches.ExpandAndGetRef().m_Data = ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], i - uiCurrentBatchStartIndex);

        uiCurrentBatchId = pRenderData->m_uiBatchId;
        uiCurrentBatchStartIndex = i;
        pCurrentBatchType = pRenderData->GetDynamicRTTI();
      }
    }

    dataPerCategory.m_Batches.ExpandAndGetRef().m_Data =
        ezMakeArrayPtr(&data[uiCurrentBatchStartIndex], data.GetCount() - uiCurrentBatchStartIndex);
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

  // TODO: intelligent compact
}

ezRenderDataBatchList ezExtractedRenderData::GetRenderDataBatchesWithCategory(ezRenderData::Category category,
                                                                              ezRenderDataBatch::Filter filter) const
{
  if (category.m_uiValue < m_DataPerCategory.GetCount())
  {
    ezRenderDataBatchList list;
    list.m_Batches = m_DataPerCategory[category.m_uiValue].m_Batches;
    list.m_Filter = filter;

    return list;
  }

  return ezRenderDataBatchList();
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

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ExtractedRenderData);

