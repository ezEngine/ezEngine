#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>

class ezProfilingId;
class ezView;
class ezRenderPipelinePass;
class ezFrameDataProviderBase;

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

  // \brief Resets the pipeline state to 'Uninitialized' to force a recompute (e.g. when the render target has changed).
  void ResetPipelineState();

  void AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass);
  void RemovePass(ezRenderPipelinePass* pPass);
  void GetPasses(ezHybridArray<const ezRenderPipelinePass*, 16>& passes) const;
  void GetPasses(ezHybridArray<ezRenderPipelinePass*, 16>& passes);
  ezRenderPipelinePass* GetPassByName(const ezStringView& sPassName);
  
  bool Connect(ezRenderPipelinePass* pOutputNode, const char* szOutputPinName, ezRenderPipelinePass* pInputNode, const char* szInputPinName);
  bool Connect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);
  bool Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);
  
  const ezRenderPipelinePassConnection* GetInputConnection(ezRenderPipelinePass* pPass, ezHashedString sInputPinName) const;
  const ezRenderPipelinePassConnection* GetOutputConnection(ezRenderPipelinePass* pPass, ezHashedString sOutputPinName) const;

  void AddExtractor(ezUniquePtr<ezExtractor>&& pExtractor);
  void RemoveExtractor(ezExtractor* pExtractor);
  void GetExtractors(ezHybridArray<const ezExtractor*, 16>& extractors) const;
  void GetExtractors(ezHybridArray<ezExtractor*, 16>& extractors);
  ezExtractor* GetExtractorByName(const ezStringView& sExtractorName);

  template <typename T>
  EZ_FORCE_INLINE T* GetFrameDataProvider() { return static_cast<T*>(GetFrameDataProvider(ezGetStaticRTTI<T>())); }

  const ezExtractedRenderData& GetRenderData() const;
  ezRenderDataBatchList GetRenderDataBatchesWithCategory(ezRenderData::Category category, ezRenderDataBatch::Filter filter = ezRenderDataBatch::Filter()) const;

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  friend class ezRenderLoop;
  friend class ezView;  

  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild(const ezView& view);
  bool RebuildInternal(const ezView& view);
  bool SortPasses();
  bool InitRenderTargetDescriptions(const ezView& view);
  bool CreateRenderTargetUsage(const ezView& view);
  bool InitRenderPipelinePasses();

  void RemoveConnections(ezRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const;
  bool ArePassThroughInputsDone(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const;

  ezFrameDataProviderBase* GetFrameDataProvider(const ezRTTI* pRtti);

  void ExtractData(const ezView& view);
  void Render(ezRenderContext* pRenderer);

private: // Member data
  // Thread data
  ezThreadID m_CurrentExtractThread;
  ezThreadID m_CurrentRenderThread;

  // Pipeline render data
  ezExtractedRenderData m_Data[2];

  // Profiling
  ezProfilingId m_RenderProfilingID;
  ezUInt64 m_uiLastExtractionFrame;
  ezUInt64 m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState;
  
  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Inputs;
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Outputs;
  };
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> m_Passes;
  ezMap<const ezRenderPipelinePass*, ConnectionData> m_Connections;

  /// \brief Contains all connections that share the same path-through texture and their first and last usage pass index.
  struct TextureUsageData
  {
    ezHybridArray<ezRenderPipelinePassConnection*, 4> m_UsedBy;
    ezUInt16 m_uiFirstUsageIdx;
    ezUInt16 m_uiLastUsageIdx;
    bool m_bTargetTexture;
  };
  ezDynamicArray<TextureUsageData> m_TextureUsage;
  ezDynamicArray<ezUInt16> m_TextureUsageIdxSortedByFirstUsage; ///< Indices map into m_TextureUsage
  ezDynamicArray<ezUInt16> m_TextureUsageIdxSortedByLastUsage; ///< Indices map into m_TextureUsage

  // Extractors
  ezDynamicArray<ezUniquePtr<ezExtractor>> m_Extractors;

  // Data Providers
  ezDynamicArray<ezUniquePtr<ezFrameDataProviderBase>> m_DataProviders;
  ezHashTable<const ezRTTI*, ezUInt32> m_TypeToDataProviderIndex;
};
