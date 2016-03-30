#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

class EZ_RENDERERCORE_DLL ezExtractedRenderData
{
public:

  ezExtractedRenderData();

  EZ_FORCE_INLINE void SetCamera(const ezCamera& camera)
  {
    m_Camera = camera;
  }

  EZ_FORCE_INLINE const ezCamera& GetCamera() const
  {
    return m_Camera;
  }

  EZ_FORCE_INLINE void SetViewData(const ezViewData& viewData)
  {
    m_ViewData = viewData;
  }

  EZ_FORCE_INLINE const ezViewData& GetViewData() const
  {
    return m_ViewData;
  }

  EZ_FORCE_INLINE void SetWorldIndex(ezUInt32 uiWorldIndex)
  {
    m_uiWorldIndex = uiWorldIndex;
  }

  EZ_FORCE_INLINE ezUInt32 GetWorldIndex() const
  {
    return m_uiWorldIndex;
  }

  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezUInt32 uiRenderDataSortingKey);

  void SortAndBatch();

  void Clear();

  ezArrayPtr< const ezRenderDataBatch > GetRenderDataBatchesWithCategory(ezRenderData::Category category) const;

private:

  struct DataPerCategory
  {
    ezDynamicArray< ezRenderDataBatch > m_Batches;
    ezDynamicArray< ezRenderDataBatch::SortableRenderData > m_SortableRenderData;
  };

  ezCamera m_Camera;
  ezViewData m_ViewData;
  ezUInt32 m_uiWorldIndex;

  ezHybridArray< DataPerCategory, 8 > m_DataPerCategory;
};
