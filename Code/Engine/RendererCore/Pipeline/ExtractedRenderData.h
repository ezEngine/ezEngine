#pragma once

#include <Core/Graphics/Camera.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererCore/Declarations.h>
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

  /// \name Initial setup
  ///@{

  EZ_ALWAYS_INLINE void SetCamera(const ezCamera& camera) { m_Camera = camera; }
  EZ_ALWAYS_INLINE const ezCamera& GetCamera() const { return m_Camera; }

  EZ_ALWAYS_INLINE void SetViewData(const ezViewData& viewData) { m_ViewData = viewData; }
  EZ_ALWAYS_INLINE const ezViewData& GetViewData() const { return m_ViewData; }

  EZ_ALWAYS_INLINE void SetWorldHandle(const ezWorldHandle& hWorld) { m_hWorld = hWorld; }
  EZ_ALWAYS_INLINE const ezWorldHandle& GetWorldHandle() const { return m_hWorld; }

  EZ_ALWAYS_INLINE void SetWorldTime(ezTime time) { m_WorldTime = time; }
  EZ_ALWAYS_INLINE ezTime GetWorldTime() const { return m_WorldTime; }

  EZ_ALWAYS_INLINE void SetWorldDebugContext(const ezDebugRendererContext& debugContext) { m_WorldDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetWorldDebugContext() const { return m_WorldDebugContext; }

  EZ_ALWAYS_INLINE void SetViewDebugContext(const ezDebugRendererContext& debugContext) { m_ViewDebugContext = debugContext; }
  EZ_ALWAYS_INLINE const ezDebugRendererContext& GetViewDebugContext() const { return m_ViewDebugContext; }

  ///@}
  /// \name Add extracted data
  ///@{

  /// Adds render data for a specific rendering category.
  EZ_ALWAYS_INLINE void AddRenderData(const ezRenderData* pRenderData, ezRenderData::Category category);

  /// Adds frame-level data that is not tied to a specific render category.
  EZ_ALWAYS_INLINE void AddFrameData(const ezRenderData* pFrameData);

  EZ_ALWAYS_INLINE void AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  EZ_ALWAYS_INLINE void AddViewDependency(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Adds a texture barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call ezMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  EZ_ALWAYS_INLINE void AddDependency(const ezTextureDependency& dependency);

  /// Adds a buffer barrier dependency that applies when its category is rendered. The category is taken from the dependency itself.
  /// Should not be called in user code, call ezMsgExtractRenderData::AddDependency instead during extraction of object or call AddViewDependency.
  EZ_ALWAYS_INLINE void AddDependency(const ezBufferDependency& dependency);

  /// Adds a sampler that is added to the EZ_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa ezBindGroupBuilder
  void AddSamplerBinding(ezTempHashedString sSlotName, ezGALSamplerStateHandle hSampler);
  EZ_ALWAYS_INLINE void AddSamplerBinding(const ezSamplerBinding& binding);

  /// Adds a buffer that is added to the EZ_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa ezBindGroupBuilder
  void AddBufferBinding(ezTempHashedString sSlotName, ezGALBufferHandle hBuffer, ezGALBufferRange bufferRange = {}, ezEnum<ezGALResourceFormat> overrideTexelBufferFormat = ezGALResourceFormat::Invalid);
  EZ_ALWAYS_INLINE void AddBufferBinding(const ezBufferBinding& binding);

  /// Adds a texture that is added to the EZ_GAL_BIND_GROUP_FRAME during rendering.
  /// \sa ezBindGroupBuilder
  void AddTextureBinding(ezTempHashedString sSlotName, ezGALTextureHandle hTexture, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid, ezEnum<ezGALTextureType> overrideViewType = ezGALTextureType::Invalid);
  EZ_ALWAYS_INLINE void AddTextureBinding(const ezTextureBinding& binding);

  void AddTextureBinding(ezTempHashedString sSlotName, const ezTexture2DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid, ezEnum<ezGALTextureType> overrideViewType = ezGALTextureType::Invalid);
  void AddTextureBinding(ezTempHashedString sSlotName, const ezTexture3DResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid, ezEnum<ezGALTextureType> overrideViewType = ezGALTextureType::Invalid);
  void AddTextureBinding(ezTempHashedString sSlotName, const ezTextureCubeResourceHandle& hTexture, ezResourceAcquireMode acquireMode = ezResourceAcquireMode::AllowLoadingFallback, ezGALTextureRange textureRange = {}, ezEnum<ezGALResourceFormat> overrideViewFormat = ezGALResourceFormat::Invalid, ezEnum<ezGALTextureType> overrideViewType = ezGALTextureType::Invalid);

  ///@}
  /// \name Read extracted data
  ///@{

  EZ_ALWAYS_INLINE ezArrayPtr<const ezTextureDependency> GetTextureViewDependencies() const { return m_ViewTextureDependencies; }
  EZ_ALWAYS_INLINE ezArrayPtr<const ezBufferDependency> GetBufferViewDependencies() const { return m_ViewBufferDependencies; }

  EZ_ALWAYS_INLINE ezArrayPtr<const ezSamplerBinding> GetSamplerBindings() const { return m_SamplerBindings; }
  EZ_ALWAYS_INLINE ezArrayPtr<const ezBufferBinding> GetBufferBindings() const { return m_BufferBindings; }
  EZ_ALWAYS_INLINE ezArrayPtr<const ezTextureBinding> GetTextureBindings() const { return m_TextureBindings; }

  /// Returns the texture barrier dependencies recorded for a specific category.
  ezArrayPtr<const ezTextureDependency> GetTextureDependenciesWithCategory(ezRenderData::Category category) const;

  /// Returns the buffer barrier dependencies recorded for a specific category.
  ezArrayPtr<const ezBufferDependency> GetBufferDependenciesWithCategory(ezRenderData::Category category) const;

  /// Returns all render data batches for a specific category.
  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category) const;

  /// Returns raw unsorted render data for a specific category.
  ezArrayPtr<const ezRenderDataBatch::SortableRenderData> GetRawRenderDataWithCategory(ezRenderData::Category category) const;

  template <typename T>
  EZ_ALWAYS_INLINE const T* GetFrameData() const
  {
    return static_cast<const T*>(GetFrameData(ezGetStaticRTTI<T>()));
  }

  ///@}
  /// \name Administration
  ///@{

  /// Sorts and batches all render data by category and sorting key for efficient rendering.
  void SortAndBatch();

  void Clear();

  ///@}

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
  ezWorldHandle m_hWorld;
  ezTime m_WorldTime;

  ezDebugRendererContext m_WorldDebugContext;
  ezDebugRendererContext m_ViewDebugContext;

  ezHybridArray<DataPerCategory, 32> m_DataPerCategory;
  ezHybridArray<const ezRenderData*, 16> m_FrameData;
  ezHybridArray<ezTextureDependency, 4> m_ViewTextureDependencies;
  ezHybridArray<ezBufferDependency, 4> m_ViewBufferDependencies;

  ezHybridArray<ezSamplerBinding, 2> m_SamplerBindings;
  ezHybridArray<ezBufferBinding, 2> m_BufferBindings;
  ezHybridArray<ezTextureBinding, 2> m_TextureBindings;
};

#include <RendererCore/Pipeline/Implementation/ExtractedRenderData_inl.h>
