#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

class EZ_RENDERERCORE_DLL ezExtractedRenderData
{
public:
  ezExtractedRenderData();

  EZ_ALWAYS_INLINE void SetCamera(const ezCamera& camera) { m_Camera = camera; }
  EZ_ALWAYS_INLINE const ezCamera& GetCamera() const { return m_Camera; }

  EZ_ALWAYS_INLINE void SetLodCamera(const ezCamera& camera) { m_LodCamera = camera; }
  EZ_ALWAYS_INLINE const ezCamera& GetLodCamera() const { return m_LodCamera; }

  EZ_ALWAYS_INLINE void SetViewData(const ezViewData& viewData) { m_ViewData = viewData; }
  EZ_ALWAYS_INLINE const ezViewData& GetViewData() const { return m_ViewData; }

  EZ_ALWAYS_INLINE void SetWorldTime(ezTime time) { m_WorldTime = time; }
  EZ_ALWAYS_INLINE ezTime GetWorldTime() const { return m_WorldTime; }

  EZ_ALWAYS_INLINE void SetWorldDebugContext(const ezDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  EZ_ALWAYS_INLINE void SetViewDebugContext(const ezDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category);
  void AddFrameData(const ezRenderData* pFrameData);

  void SortAndBatch();

  void Clear();

  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category) const;

  ezArrayPtr<const ezRenderDataBatch::SortableRenderData> GetRawRenderDataWithCategory(ezRenderData::Category category) const;

  template <typename T>
  EZ_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(ezGetStaticRTTI<T>()));
  }

private:
  const ezRenderData* GetFrameData(const ezRTTI* pRtti) const;

  struct DataPerCategory
  {
    ezDynamicArray<ezRenderDataBatch> m_Batches;
    ezDynamicArray<ezRenderDataBatch::SortableRenderData> m_SortableRenderData;
  };

  ezCamera m_Camera;
  ezCamera m_LodCamera; // Temporary until we have a real LOD system
  ezViewData m_ViewData;
  ezTime m_WorldTime;

  ezDebugRendererContext m_WorldDebugContext;
  ezDebugRendererContext m_ViewDebugContext;

  ezHybridArray<DataPerCategory, 32> m_DataPerCategory;
  ezHybridArray<const ezRenderData*, 16> m_FrameData;
};
