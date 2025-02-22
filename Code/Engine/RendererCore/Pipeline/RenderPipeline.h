#pragma once

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

class ezProfilingId;
class ezView;
class ezRenderPipelinePass;
class ezFrameDataProviderBase;
struct ezPermutationVar;
class ezDGMLGraph;
class ezFrustum;
class ezRasterizerView;

class EZ_RENDERERCORE_DLL ezRenderPipeline : public ezRefCounted
{
public:
  enum class PipelineState
  {
    Uninitialized,
    RebuildError,
    Initialized
  };

  ezRenderPipeline();
  ~ezRenderPipeline();

  void AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass);
  void RemovePass(ezRenderPipelinePass* pPass);
  void GetPasses(ezDynamicArray<const ezRenderPipelinePass*>& ref_passes) const;
  void GetPasses(ezDynamicArray<ezRenderPipelinePass*>& ref_passes);
  ezRenderPipelinePass* GetPassByName(const ezStringView& sPassName);
  ezHashedString GetViewName() const;

  bool Connect(ezRenderPipelinePass* pOutputNode, const char* szOutputPinName, ezRenderPipelinePass* pInputNode, const char* szInputPinName);
  bool Connect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);
  bool Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);

  const ezRenderPipelinePassConnection* GetInputConnection(const ezRenderPipelinePass* pPass, ezHashedString sInputPinName) const;
  const ezRenderPipelinePassConnection* GetOutputConnection(const ezRenderPipelinePass* pPass, ezHashedString sOutputPinName) const;

  void AddExtractor(ezUniquePtr<ezExtractor>&& pExtractor);
  void RemoveExtractor(ezExtractor* pExtractor);
  void GetExtractors(ezDynamicArray<const ezExtractor*>& ref_extractors) const;
  void GetExtractors(ezDynamicArray<ezExtractor*>& ref_extractors);
  ezExtractor* GetExtractorByName(const ezStringView& sExtractorName);

  template <typename T>
  EZ_ALWAYS_INLINE T* GetFrameDataProvider() const
  {
    return static_cast<T*>(GetFrameDataProvider(ezGetStaticRTTI<T>()));
  }

  const ezExtractedRenderData& GetRenderData() const;
  ezRenderDataBatchList GetRenderDataBatchesWithCategory(
    ezRenderData::Category category, ezRenderDataBatch::Filter filter = ezRenderDataBatch::Filter()) const;

  /// \brief Creates a DGML graph of all passes and textures. Can be used to verify that no accidental temp textures are created due to poorly constructed pipelines or errors in code.
  void CreateDgmlGraph(ezDGMLGraph& ref_graph);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  static ezCVarBool cvar_SpatialCullingVis;
#endif

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  friend class ezRenderWorld;
  friend class ezView;

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const ezView& view);
  bool RebuildInternal(const ezView& view);
  bool SortPasses();
  bool InitRenderTargetDescriptions(const ezView& view);
  bool CreateRenderTargetUsage(const ezView& view);
  bool InitRenderPipelinePasses();
  void SortExtractors();
  void UpdateViewData(const ezView& view, ezUInt32 uiDataIndex);

  void RemoveConnections(ezRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass, const ezHybridArray<ezRenderPipelinePass*, 32>& done) const;
  bool ArePassThroughInputsDone(const ezRenderPipelinePass* pPass, const ezHybridArray<ezRenderPipelinePass*, 32>& done) const;

  ezFrameDataProviderBase* GetFrameDataProvider(const ezRTTI* pRtti) const;

  void ExtractData(const ezView& view);
  void FindVisibleObjects(const ezView& view);

  void Render(ezRenderContext* pRenderer);

  ezRasterizerView* PrepareOcclusionCulling(const ezFrustum& frustum, const ezView& view);
  void PreviewOcclusionBuffer(const ezRasterizerView& rasterizer, const ezView& view);

private: // Member data
  // Thread data
  ezThreadID m_CurrentExtractThread;
  ezThreadID m_CurrentRenderThread;

  // Pipeline render data
  ezExtractedRenderData m_Data[2];
  ezDynamicArray<const ezGameObject*> m_VisibleObjects;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezTime m_AverageCullingTime;
#endif

  ezHashedString m_sName;
  ezUInt64 m_uiLastExtractionFrame;
  ezUInt64 m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState = PipelineState::Uninitialized;

  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Inputs;
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Outputs;
  };
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> m_Passes;       ///< The passes present in the pipeline on no particular order.
  ezMap<const ezRenderPipelinePass*, ConnectionData> m_Connections; ///< Connections on each pass.

  /// \brief Contains all connections that share the same path-through texture and their first and last usage pass index.
  struct TextureUsageData
  {
    ezHybridArray<ezRenderPipelinePassConnection*, 4> m_UsedBy;  ///< All the connections that use this texture. Due to passthrough pins, this can be larger than 1.
    ezUInt16 m_uiFirstUsageIdx;                                  ///< Used to decide when to acquire a temp texture.
    ezUInt16 m_uiLastUsageIdx;                                   ///< Used to decide when to return a temp texture.
    const ezRenderPipelineNodePin* m_pTextureProvider = nullptr; ///< If set, this node and parent pass provide an external texture to the pipeline. This could be a render target from an ezTargetPass or a history buffer that is preserved across frames. At the start of every frame the parent pass will be asked for the current value of the texture a this pin.
  };
  ezDynamicArray<TextureUsageData> m_TextureUsage;               ///< All unique textures used during the pipeline run.
  ezDynamicArray<ezUInt16> m_TextureUsageIdxSortedByFirstUsage;  ///< Indices map into m_TextureUsage
  ezDynamicArray<ezUInt16> m_TextureUsageIdxSortedByLastUsage;   ///< Indices map into m_TextureUsage

  ezHashTable<ezRenderPipelinePassConnection*, ezUInt32> m_ConnectionToTextureIndex;

  // Extractors
  ezDynamicArray<ezUniquePtr<ezExtractor>> m_Extractors;
  ezDynamicArray<ezUniquePtr<ezExtractor>> m_SortedExtractors;

  // Data Providers
  mutable ezDynamicArray<ezUniquePtr<ezFrameDataProviderBase>> m_DataProviders;
  mutable ezHashTable<const ezRTTI*, ezUInt32> m_TypeToDataProviderIndex;

  ezDynamicArray<ezPermutationVar> m_PermutationVars;

  // Occlusion Culling
  ezGALTextureHandle m_hOcclusionDebugViewTexture;
};
