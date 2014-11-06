#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>

class ezWorld;
class ezCamera;

class EZ_RENDERERCORE_DLL ezRenderPipeline
{
public:
  enum Mode
  {
    Synchronous,
    Asynchronous
  };

  ezRenderPipeline(Mode mode);
  ~ezRenderPipeline();

  void ExtractData(const ezWorld& world, const ezCamera& camera);
  void Render(const ezCamera& camera, ezGALContext* pContext);

  void AddPass(ezRenderPipelinePass* pPass);
  void RemovePass(ezRenderPipelinePass* pPass);

  EZ_FORCE_INLINE void SetViewport(const ezRectFloat& viewport)
  {
    m_ViewPortRect = viewport;
  }

  EZ_FORCE_INLINE const ezRectFloat& GetViewport() const
  {
    return m_ViewPortRect;
  }

  EZ_FORCE_INLINE const ezMat4& GetViewMatrix() const
  {
    return m_ViewMatrix;
  }

  EZ_FORCE_INLINE const ezMat4& GetProjectionMatrix() const
  {
    return m_ProjectionMatrix;
  }

  EZ_FORCE_INLINE const ezMat4& GetViewProjectionMatrix() const
  {
    return m_ViewProjectionMatrix;
  }

  EZ_FORCE_INLINE const ezCamera* GetCurrentCamera() const
  {
    return m_pCurrentCamera;
  }

  EZ_FORCE_INLINE ezGALContext* GetCurrentContext()
  {
    return m_pCurrentContext;
  }

  template <typename T>
  T* CreateRenderData(ezRenderPassType passType, ezGameObject* pOwner)
  {
    EZ_CHECK_AT_COMPILETIME(EZ_IS_DERIVED_FROM_STATIC(ezRenderData, T));

    T* pRenderData = EZ_DEFAULT_NEW(T);
    pRenderData->m_uiSortingKey = 0; /// \todo implement sorting
    pRenderData->m_pOwner = pOwner;
    
    if (passType >= m_pExtractedData->m_PassData.GetCount())
    {
      m_pExtractedData->m_PassData.SetCount(passType + 1);
    }

    m_pExtractedData->m_PassData[passType].m_RenderData.PushBack(pRenderData);

    return pRenderData;
  }

  EZ_FORCE_INLINE ezArrayPtr<const ezRenderData*> GetRenderDataWithPassType(ezRenderPassType passType)
  {
    ezArrayPtr<const ezRenderData*> renderData;

    if (m_pRenderData->m_PassData.GetCount() > passType)
    {
      renderData = m_pRenderData->m_PassData[passType].m_RenderData;
    }

    return renderData;
  }

  static ezRenderPassType RegisterPassType(const char* szPassTypeName);

  EZ_FORCE_INLINE static const char* GetPassTypeName(ezRenderPassType passType)
  {
    return s_PassTypeData[passType].m_sName.GetString().GetData();
  }

  EZ_FORCE_INLINE static ezProfilingId& GetPassTypeProfilingID(ezRenderPassType passType)
  {
    return s_PassTypeData[passType].m_ProfilingID;
  }

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

  Mode m_Mode;

  PipelineData m_Data[2];

  PipelineData* m_pExtractedData;
  PipelineData* m_pRenderData;

  static void ClearPipelineData(PipelineData* pPipeLineData);

  ezDynamicArray<ezRenderPipelinePass*> m_Passes;

  ezRectFloat m_ViewPortRect;
  ezMat4 m_ViewMatrix;
  ezMat4 m_ProjectionMatrix;
  ezMat4 m_ViewProjectionMatrix;

  const ezCamera* m_pCurrentCamera;
  ezGALContext* m_pCurrentContext;

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
};

