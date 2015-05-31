#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>

class ezRenderContext;

class EZ_RENDERERCORE_DLL ezRenderPipeline
{
public:
  ezRenderPipeline();
  ~ezRenderPipeline();

  void ExtractData(const ezView& view);
  void Render(const ezView& view, ezRenderContext* pRenderer);

  void AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass);
  void RemovePass(ezUniquePtr<ezRenderPipelinePass>&& pPass);

  template <typename T>
  T* CreateRenderData(ezRenderPassType passType, ezGameObject* pOwner)
  {
    EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

    T* pRenderData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), T);
    pRenderData->m_uiSortingKey = 0; /// \todo implement sorting

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    pRenderData->m_pOwner = pOwner;
  #endif
    
    PipelineData* pPipelineData = GetPipelineDataForExtraction();
    if (passType >= pPipelineData->m_PassData.GetCount())
    {
      pPipelineData->m_PassData.SetCount(passType + 1);
    }

    pPipelineData->m_PassData[passType].m_RenderData.PushBack(pRenderData);

    return pRenderData;
  }

  EZ_FORCE_INLINE ezArrayPtr<const ezRenderData*> GetRenderDataWithPassType(ezRenderPassType passType)
  {
    ezArrayPtr<const ezRenderData*> renderData;

    PipelineData* pPipelineData = GetPipelineDataForRendering();
    if (pPipelineData->m_PassData.GetCount() > passType)
    {
      renderData = pPipelineData->m_PassData[passType].m_RenderData;
    }

    return renderData;
  }

  static ezRenderPassType FindOrRegisterPassType(const char* szPassTypeName);

  EZ_FORCE_INLINE static const char* GetPassTypeName(ezRenderPassType passType)
  {
    return s_PassTypeData[passType].m_sName.GetString().GetData();
  }

  EZ_FORCE_INLINE static ezProfilingId& GetPassTypeProfilingID(ezRenderPassType passType)
  {
    return s_PassTypeData[passType].m_ProfilingID;
  }

  EZ_DISALLOW_COPY_AND_ASSIGN(ezRenderPipeline);

private:
  struct PassData
  {
    void SortRenderData();

    ezDynamicArray<const ezRenderData*> m_RenderData;
  };

  struct PipelineData
  {
    ezHybridArray<PassData, 8> m_PassData;
  };

  PipelineData m_Data[2];

  PipelineData* GetPipelineDataForExtraction();
  PipelineData* GetPipelineDataForRendering();

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
    MAX_PASS_TYPES = sizeof(ezRenderPassType) * 8
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

