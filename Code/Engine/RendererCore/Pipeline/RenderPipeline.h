#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/ViewData.h>
#include <Core/World/GameObject.h>

ezView;

class EZ_RENDERERCORE_DLL ezRenderPipeline
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

  // \brief Returns the current state of the pipeline. Evaluated via Rebuild.
  PipelineState GetPipelineState() const { return m_PipelineState; }

  // \brief Resets the pipeline state to 'Uninitialized' to force a recompute (e.g. when the render target has changed).
  void ResetPipelineState();

  void AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass);
  void RemovePass(ezRenderPipelinePass* pPass);
  void GetPasses(ezHybridArray<ezRenderPipelinePass*, 16>& passes);

  bool Connect(ezRenderPipelinePass* pOutputNode, const char* szOutputPinName, ezRenderPipelinePass* pInputNode, const char* szInputPinName);
  bool Connect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);
  bool Disconnect(ezRenderPipelinePass* pOutputNode, ezHashedString sOutputPinName, ezRenderPipelinePass* pInputNode, ezHashedString sInputPinName);
  
  // \brief Rebuilds the render pipeline, e.g. sorting passes via dependencies and creating render targets.
  PipelineState Rebuild();

  void AddExtractor(ezUniquePtr<ezExtractor>&& pExtractor);
  void RemoveExtractor(ezExtractor* pExtractor);
  void GetExtractors(ezHybridArray<ezExtractor*, 16>& extractors);

  // \brief Returns the view this pipeline is bound to.
  ezView* GetView() { return m_pView; }

  template <typename T>
  T* CreateRenderData(ezRenderPassType passType, const ezGameObject* pOwner);
  
  ezArrayPtr<const ezRenderData* const> GetRenderDataWithPassType(ezRenderPassType passType) const;

  static ezRenderPassType FindOrRegisterPassType(const char* szPassTypeName);

  static const char* GetPassTypeName(ezRenderPassType passType);
  static ezProfilingId& GetPassTypeProfilingID(ezRenderPassType passType);

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  friend class ezRenderLoop;
  friend class ezView;  

  struct PassData
  {
    void SortRenderData();

    ezDynamicArray<const ezRenderData*> m_RenderData;
  };

  struct PipelineData
  {
    ezCamera m_Camera;
    ezViewData m_ViewData;

    ezHybridArray<PassData, 8> m_PassData;
  };

  bool RebuildInternal();
  bool SortPasses();
  bool InitRenderTargetDescriptions();
  bool CreateRenderTargets();
  bool SetRenderTargets();

  void SetView(ezView* pView);
  void RemoveConnections(ezRenderPipelinePass* pPass);
  void ClearRenderPassGraphTextures();
  bool AreInputDescriptionsAvailable(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const;
  bool ArePassThroughInputsDone(const ezRenderPipelinePass* pPass, const ezDynamicArray<ezRenderPipelinePass*>& done) const;

  void ExtractData(const ezView& view);
  void Render(ezRenderContext* pRenderer);

  PipelineData* GetPipelineDataForExtraction();
  PipelineData* GetPipelineDataForRendering();
  const PipelineData* GetPipelineDataForRendering() const;

private: // Static functions
  static void ClearPipelineData(PipelineData* pPipeLineData);
  static bool IsPipelineDataEmpty(PipelineData* pPipeLineData);

private: // Member data
  // Thread data
  ezThreadID m_CurrentExtractThread;
  ezThreadID m_CurrentRenderThread;

  // Pipeline render data
  PipelineData m_Data[2];

  // Profiling
  ezProfilingId m_RenderProfilingID;
  ezUInt32 m_uiLastExtractionFrame;
  ezUInt32 m_uiLastRenderFrame;

  // Render pass graph data
  PipelineState m_PipelineState;
  ezView* m_pView; ///< The view that uses this render pipeline. Needed to query view size, render target setup etc.
  
  struct ConnectionData
  {
    // Inputs / outputs match the node pin indices. Value at index is nullptr if not connected.
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Inputs;
    ezDynamicArray<ezRenderPipelinePassConnection*> m_Outputs;
  };
  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> m_Passes;
  ezMap<const ezRenderPipelinePass*, ConnectionData> m_Connections;
  ezDynamicArray<ezGALTextureHandle> m_Textures; ///< All unique textures created for the entire pipeline, may be used multiple times in the pipeline.

  // Extractors
  ezDynamicArray<ezUniquePtr<ezExtractor>> m_Extractors;

private: // Static data
  struct PassTypeData
  {
    ezHashedString m_sName;
    ezProfilingId m_ProfilingID;
  };

  enum
  {
    MAX_PASS_TYPES = 32
  };
  static ezRenderPassType s_uiNextPassType;
  static PassTypeData s_PassTypeData[MAX_PASS_TYPES];
};

class EZ_RENDERERCORE_DLL ezDefaultPassTypes
{
public:
  static ezRenderPassType Opaque;
  static ezRenderPassType Masked;
  static ezRenderPassType Transparent;
  static ezRenderPassType Foreground1;
  static ezRenderPassType Foreground2;
  static ezRenderPassType Selection;
};

#include <RendererCore/Pipeline/Implementation/RenderPipeline_inl.h>
