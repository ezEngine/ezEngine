#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/RenderPipelinePassGraph.h>

class ezProfilingId;
class ezView;
class ezCamera;
struct ezViewData;
class ezRenderPipelinePass;
class ezFrameDataProviderBase;
struct ezPermutationVar;
class ezDGMLGraph;
class ezFrustum;
class ezRasterizerView;
class ezRenderGraph;
struct ezRenderGraphRenderEvent;
class ezRenderGraphContext;

class EZ_RENDERERCORE_DLL ezRenderPipeline : public ezRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized,
    RenderGraphBuilt,
  };

  ezRenderPipeline(ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>>&& passes, ezDynamicArray<ezUniquePtr<ezExtractor>>&& extractors, ezArrayPtr<const ezRenderPipelineResourceLoaderConnection> connections);
  ~ezRenderPipeline();

  void GetPasses(ezDynamicArray<const ezRenderPipelinePass*>& ref_passes) const;
  void GetPasses(ezDynamicArray<ezRenderPipelinePass*>& ref_passes);
  ezRenderPipelinePass* GetPassByName(const ezStringView& sPassName);
  ezHashedString GetViewName() const;

  void GetExtractors(ezDynamicArray<const ezExtractor*>& ref_extractors) const;
  void GetExtractors(ezDynamicArray<ezExtractor*>& ref_extractors);
  ezExtractor* GetExtractorByName(const ezStringView& sExtractorName);

  ezArrayPtr<const ezRenderPipelinePassGraph::SwitchInfo> GetSwitches() const { return m_PassGraph.GetSwitches(); }
  bool SetSwitchValue(ezUInt32 uiSwitchIndex, ezInt32 iValue) { return m_PassGraph.SetSwitchValue(uiSwitchIndex, iValue); }
  bool SetSwitchToDefault(ezUInt32 uiSwitchIndex) { return m_PassGraph.SetSwitchToDefault(uiSwitchIndex); }

  template <typename T>
  EZ_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(ezGetStaticRTTI<T>()));
  }

  const ezExtractedRenderData& GetRenderData() const;
  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category) const;

  /// Returns the texture barrier dependencies recorded during extraction for a specific category.
  ezArrayPtr<const ezTextureDependency> GetTextureDependenciesWithCategory(ezRenderData::Category category) const;

  /// Returns the buffer barrier dependencies recorded during extraction for a specific category.
  ezArrayPtr<const ezBufferDependency> GetBufferDependenciesWithCategory(ezRenderData::Category category) const;

  /// Adds a texture dependency to the current extraction frame's data.
  /// Must only be called during extraction.
  void AddViewDependency(ezGALTextureHandle hTexture, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  /// Adds a buffer dependency to the current extraction frame's data.
  /// Must only be called during extraction.
  void AddViewDependency(ezGALBufferHandle hBuffer, ezBitflags<ezGALResourceState> requiredState, ezBitflags<ezGALShaderStageFlags> stage = ezGALShaderStageFlags::Auto);

  using RenderDataProcessor = ezDelegate<void(ezExtractedRenderData&)>;
  ezUInt32 AddRenderDataProcessor(RenderDataProcessor processor);

  /// Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(ezDGMLGraph& ref_graph);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static ezCVarBool cvar_SpatialCullingVis;
#endif

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  friend class ezRenderWorld;
  friend class ezView;

  /// Returns whether the pipeline should be rendered. E.g. returns false if the associated world has been destroyed.
  bool ShouldRender() const;

  // Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const ezView& view);
  bool RebuildInternal(const ezView& view);
  bool RebuildRenderGraph(const ezViewData& viewData, const ezCamera& camera);
  bool AddRenderPasses(const ezViewData& viewData, const ezCamera& camera);
  bool UpdateTextureProviders();
  void UpdateViewData(const ezView& view, ezUInt32 uiDataIndex);

  ezFrameDataProviderBase* GetFrameDataProvider(const ezRTTI* pRtti) const;

  void ExtractData(const ezView& view);
  void FindVisibleObjects(const ezView& view);

  void EnqueueRenderGraph(ezRenderContext* pRenderer);
  void UpdateRenderContext(ezRenderGraphContext& ctx);

  ezRasterizerView* PrepareOcclusionCulling(const ezFrustum& frustum, const ezView& view);
  void PreviewOcclusionBuffer(const ezRasterizerView& rasterizer, const ezView& view);

  void OnRenderEvent(const ezRenderGraphRenderEvent& e);

private: // Member data
  // Thread data
  ezThreadID m_CurrentExtractThread = (ezThreadID)0;
  ezThreadID m_CurrentRenderThread = (ezThreadID)0;

  // Pipeline render data
  ezExtractedRenderData m_Data[2];
  ezDynamicArray<const ezGameObject*> m_VisibleObjects;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezTime m_AverageCullingTime;
#endif

  ezHashedString m_sName;
  ezUInt64 m_uiLastExtractionFrame = -1;
  ezUInt64 m_uiLastRenderFrame = -1;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  ezRenderPipelinePassGraph m_PassGraph;

  /// Render Graph
  ezSharedPtr<ezRenderGraph> m_pRenderGraph;
  ezRenderViewContext m_RenderViewContext;
  ezUInt32 m_uiSettingsModificationCounter = 0;

  // Data Providers
  mutable ezDynamicArray<ezUniquePtr<ezFrameDataProviderBase>> m_DataProviders;
  mutable ezHashTable<const ezRTTI*, ezUInt32> m_TypeToDataProviderIndex;

  ezDynamicArray<RenderDataProcessor> m_RenderDataProcessors;

  ezDynamicArray<ezPermutationVar> m_PermutationVars;

  // Occlusion Culling
  ezGALTextureHandle m_hOcclusionDebugViewTexture;
};
