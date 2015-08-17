#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <CoreUtils/Graphics/Camera.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/ViewData.h>

class EZ_RENDERERCORE_DLL ezRenderPipeline
{
public:
  ezRenderPipeline();
  ~ezRenderPipeline();

  void ExtractData(const ezView& view);
  void Render(ezRenderContext* pRenderer);

  void AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass);
  void RemovePass(ezUniquePtr<ezRenderPipelinePass>&& pPass);

  void Connect(ezNode* pOutputNode, ezTempHashedString sOutputPinName, ezNode* pInputNode, ezTempHashedString sInputPinName);
  void Rebuild();


  template <typename T>
  T* CreateRenderData(ezRenderPassType passType, const ezGameObject* pOwner);
  
  ezArrayPtr<const ezRenderData* const> GetRenderDataWithPassType(ezRenderPassType passType) const;


  static ezRenderPassType FindOrRegisterPassType(const char* szPassTypeName);

  static const char* GetPassTypeName(ezRenderPassType passType);
  static ezProfilingId& GetPassTypeProfilingID(ezRenderPassType passType);

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  friend class ezView;

  ezThreadID m_CurrentExtractThread;
  ezThreadID m_CurrentRenderThread;

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

  PipelineData m_Data[2];

  PipelineData* GetPipelineDataForExtraction();
  PipelineData* GetPipelineDataForRendering();
  const PipelineData* GetPipelineDataForRendering() const;

  ezProfilingId m_RenderProfilingID;
  ezUInt32 m_uiLastExtractionFrame;
  ezUInt32 m_uiLastRenderFrame;

  static void ClearPipelineData(PipelineData* pPipeLineData);
  static bool IsPipelineDataEmpty(PipelineData* pPipeLineData);

  ezDynamicArray<ezUniquePtr<ezRenderPipelinePass>> m_Passes;

  static ezRenderPassType s_uiNextPassType;

  struct PassTypeData
  {
    ezHashedString m_sName;
    ezProfilingId m_ProfilingID;
  };

  enum
  {
    MAX_PASS_TYPES = 32
  };

  static PassTypeData s_PassTypeData[MAX_PASS_TYPES];
};

class EZ_RENDERERCORE_DLL ezDefaultPassTypes
{
public:
  static ezRenderPassType Opaque;
  static ezRenderPassType Masked;
  static ezRenderPassType Transparent;
  static ezRenderPassType Foreground;
};

#include <RendererCore/Pipeline/Implementation/RenderPipeline_inl.h>
