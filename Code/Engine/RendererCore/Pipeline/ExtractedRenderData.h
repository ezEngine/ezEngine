#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/Pipeline/ViewData.h>

/// Contains all render data extracted from a view for one frame.
///
/// During the extraction phase, render components add their render data to this container,
/// organized by category (opaque, transparent, etc.). The data is then sorted and batched
/// for efficient rendering. Also stores camera data, view data, world time, and debug contexts.
class EZ_RENDERERCORE_DLL ezExtractedRenderData
{
public:
  ezExtractedRenderData();
  ~ezExtractedRenderData();

  EZ_ALWAYS_INLINE void SetCamera(const ezCamera& camera) { m_Camera = camera; }
  EZ_ALWAYS_INLINE const ezCamera& GetCamera() const { return m_Camera; }

  EZ_ALWAYS_INLINE void SetViewData(const ezViewData& viewData) { m_ViewData = viewData; }
  EZ_ALWAYS_INLINE const ezViewData& GetViewData() const { return m_ViewData; }

  EZ_ALWAYS_INLINE void SetWorldTime(ezTime time) { m_WorldTime = time; }
  EZ_ALWAYS_INLINE ezTime GetWorldTime() const { return m_WorldTime; }

  EZ_ALWAYS_INLINE void SetWorldDebugContext(const ezDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  EZ_ALWAYS_INLINE void SetViewDebugContext(const ezDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  EZ_ALWAYS_INLINE void AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto)
  {
    auto& dep = m_ViewTextureDependencies.ExpandAndGetRef();
    dep.m_hTexture = hTexture;
    dep.m_RequiredState = requiredState;
    dep.m_Stage = stage;
  }
  EZ_ALWAYS_INLINE ezArrayPtr<const ezTextureDependency> GetTextureViewDependencies() const { return m_ViewTextureDependencies; }

  EZ_ALWAYS_INLINE void AddViewDependency(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto)
  {
    auto& dep = m_ViewBufferDependencies.ExpandAndGetRef();
    dep.m_hBuffer = hBuffer;
    dep.m_RequiredState = requiredState;
    dep.m_Stage = stage;
  }
  EZ_ALWAYS_INLINE ezArrayPtr<const ezBufferDependency> GetBufferViewDependencies() const { return m_ViewBufferDependencies; }

  /// Adds render data for a specific rendering category.
  void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category);

  /// Adds a texture barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call ezMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  void AddDependency(const ezTextureDependency& dependency);

  /// Adds a buffer barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call ezMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  void AddDependency(const ezBufferDependency& dependency);

  /// Returns the texture barrier dependencies recorded for a specific category.
  ezArrayPtr<const ezTextureDependency> GetTextureDependenciesWithCategory(ezRenderData::Category category) const;

  /// Returns the buffer barrier dependencies recorded for a specific category.
  ezArrayPtr<const ezBufferDependency> GetBufferDependenciesWithCategory(ezRenderData::Category category) const;

  /// Adds frame-level data that is not tied to a specific render category.
  void AddFrameData(const ezRenderData* pFrameData);

  /// Sorts and batches all render data by category and sorting key for efficient rendering.
  void SortAndBatch();

  void Clear();

  /// Returns all render data batches for a specific category.
  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category) const;

  /// Returns raw unsorted render data for a specific category.
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
    ezDynamicArray<ezInstanceableRenderData::DataOffsets> m_DataOffsets;
    ezGALBufferHandle m_hDataOffsetsBuffer;
    ezDynamicArray<ezTextureDependency> m_TextureDependencies;
    ezDynamicArray<ezBufferDependency> m_BufferDependencies;
  };

  void SortAndBatchCategory(DataPerCategory& dataPerCategory, ezRenderData::Category category);

  ezCamera m_Camera;
  ezViewData m_ViewData;
  ezTime m_WorldTime;

  ezDebugRendererContext m_WorldDebugContext;
  ezDebugRendererContext m_ViewDebugContext;

  ezHybridArray<DataPerCategory, 32> m_DataPerCategory;
  ezHybridArray<const ezRenderData*, 16> m_FrameData;
  ezHybridArray<ezTextureDependency, 4> m_ViewTextureDependencies;
  ezHybridArray<ezBufferDependency, 4> m_ViewBufferDependencies;
};
