#pragma once

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <Core/Graphics/Camera.h>

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

  EZ_FORCE_INLINE void SetWorldTime(ezTime time)
  {
    m_WorldTime = time;
  }

  EZ_FORCE_INLINE ezTime GetWorldTime() const
  {
    return m_WorldTime;
  }

  EZ_FORCE_INLINE void SetWorldDebugContext(const ezDebugRendererContext& debugContext)
  {
    m_WorldDebugContext = debugContext;
  }

  EZ_FORCE_INLINE const ezDebugRendererContext& GetWorldDebugContext() const
  {
    return m_WorldDebugContext;
  }

  EZ_FORCE_INLINE void SetViewDebugContext(const ezDebugRendererContext& debugContext)
  {
    m_ViewDebugContext = debugContext;
  }

  EZ_FORCE_INLINE const ezDebugRendererContext& GetViewDebugContext() const
  {
    return m_ViewDebugContext;
  }

  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category, ezUInt32 uiRenderDataSortingKey);

  void SortAndBatch();

  void Clear();

  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category, ezRenderDataBatch::Filter filter = ezRenderDataBatch::Filter()) const;

private:

  struct DataPerCategory
  {
    ezDynamicArray< ezRenderDataBatch > m_Batches;
    ezDynamicArray< ezRenderDataBatch::SortableRenderData > m_SortableRenderData;
  };

  ezCamera m_Camera;
  ezViewData m_ViewData;
  ezTime m_WorldTime;

  ezDebugRendererContext m_WorldDebugContext;
  ezDebugRendererContext m_ViewDebugContext;

  ezHybridArray< DataPerCategory, 8 > m_DataPerCategory;
};
