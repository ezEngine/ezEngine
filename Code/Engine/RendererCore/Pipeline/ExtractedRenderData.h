#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

class EZ_RENDERERCORE_DLL ezExtractedRenderData
{
public:

  ezExtractedRenderData();

  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezUInt32 uiRenderDataSortingKey);

  void SortAndBatch();

  void Clear();

private:

  friend class ezRenderPipeline;

  struct DataPerCategory
  {
    ezDynamicArray< ezRenderDataBatch > m_Batches;
    ezDynamicArray< ezRenderDataBatch::SortableRenderData > m_SortableRenderData;
  };

  ezCamera m_Camera;
  ezViewData m_ViewData;

  ezHybridArray< DataPerCategory, 8 > m_DataPerCategory;
};
