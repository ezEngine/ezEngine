#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/ViewData.h>

class EZ_RENDERERCORE_DLL ezBatchedRenderData
{
public:

  ezBatchedRenderData();

  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category);

  void SortAndBatch();

  void Clear();

private:

  friend class ezRenderPipeline;

  struct SortableRenderData
  {
    EZ_DECLARE_POD_TYPE();

    const ezRenderData* m_pRenderData;
    ezUInt64 m_uiSortingKey;
  };

  ezDynamicArray< SortableRenderData > m_TempDataForSorting;

  struct DataPerCategory
  {
    ezDynamicArray< ezArrayPtr<const ezRenderData*> > m_Batches;
    ezDynamicArray< const ezRenderData* > m_SortedRenderData;
  };

  ezCamera m_Camera;
  ezViewData m_ViewData;

  ezHybridArray< DataPerCategory, 8 > m_DataPerCategory;
};
