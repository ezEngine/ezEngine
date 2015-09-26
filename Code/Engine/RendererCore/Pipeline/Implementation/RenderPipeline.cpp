#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderLoop/RenderLoop.h>

#include <Foundation/Configuration/CVar.h>

#include <Core/World/World.h>

#include <RendererFoundation/Context/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_IMPLEMENT_MESSAGE_TYPE(ezExtractRenderDataMessage);

extern ezCVarBool CVarMultithreadedRendering;

ezRenderPassType ezRenderPipeline::s_uiNextPassType = 0;
ezRenderPipeline::PassTypeData ezRenderPipeline::s_PassTypeData[MAX_PASS_TYPES];

void ezRenderPipeline::PassData::SortRenderData()
{
  struct RenderDataComparer
  {
    EZ_FORCE_INLINE bool Less(const ezRenderData* a, const ezRenderData* b) const
    {
      return a->GetSortingKey() < b->GetSortingKey();
    }
  };

  m_RenderData.Sort(RenderDataComparer());
}

ezRenderPipeline::ezRenderPipeline()
{
  m_CurrentExtractThread = (ezThreadID)0;
  m_CurrentRenderThread = (ezThreadID)0;
  m_uiLastExtractionFrame = -1;
  m_uiLastRenderFrame = -1;
}

ezRenderPipeline::~ezRenderPipeline()
{
  ClearPipelineData(&m_Data[0]);
  ClearPipelineData(&m_Data[1]);
}

void ezRenderPipeline::ExtractData(const ezView& view)
{
  EZ_ASSERT_DEV(m_CurrentExtractThread == (ezThreadID)0, "Extract must not be called from multiple threads.");
  m_CurrentExtractThread = ezThreadUtils::GetCurrentThreadID();

  // Is this view already extracted?
  if (m_uiLastExtractionFrame == ezRenderLoop::GetFrameCounter())
    return;

  m_uiLastExtractionFrame = ezRenderLoop::GetFrameCounter();
  
  PipelineData* pPipelineData = GetPipelineDataForExtraction();

  // Usually clear is not needed, only if the multithreading flag is switched during runtime.
  ClearPipelineData(pPipelineData);

  // Store camera and viewdata
  pPipelineData->m_Camera = *view.GetRenderCamera();
  pPipelineData->m_ViewData = view.GetData();

  // Extract object render data
  {
    ezExtractRenderDataMessage msg;
    msg.m_pView = &view;

    EZ_LOCK(view.GetWorld()->GetReadMarker());

    /// \todo use spatial data to do visibility culling etc.
    for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
    {
      const ezGameObject* pObject = it;

      if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
        continue;

      if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
        continue;

      pObject->SendMessage(msg);
    }
  }

  for (ezUInt32 uiPassIndex = 0; uiPassIndex < pPipelineData->m_PassData.GetCount(); ++uiPassIndex)
  {
    PassData& data = pPipelineData->m_PassData[uiPassIndex];
    data.SortRenderData();
  }

  m_CurrentExtractThread = (ezThreadID)0;
}

void ezRenderPipeline::Render(ezRenderContext* pRendererContext)
{
  EZ_PROFILE_AND_MARKER(pRendererContext->GetGALContext(), m_RenderProfilingID);

  EZ_ASSERT_DEV(m_CurrentRenderThread == (ezThreadID)0, "Render must not be called from multiple threads.");
  m_CurrentRenderThread = ezThreadUtils::GetCurrentThreadID();

  EZ_ASSERT_DEV(m_uiLastRenderFrame != ezRenderLoop::GetFrameCounter(), "Render must not be called multiple times per frame.");
  m_uiLastRenderFrame = ezRenderLoop::GetFrameCounter();

  const PipelineData* pPipelineData = GetPipelineDataForRendering();
  const ezCamera* pCamera = &pPipelineData->m_Camera;
  const ezViewData* pViewData = &pPipelineData->m_ViewData;

  auto& gc = pRendererContext->WriteGlobalConstants();
  gc.CameraPosition = pCamera->GetPosition();
  gc.CameraDirForwards = pCamera->GetDirForwards();
  gc.CameraDirRight = pCamera->GetDirRight();
  gc.CameraDirUp = pCamera->GetDirUp();
  gc.CameraToScreenMatrix = pViewData->m_ProjectionMatrix;
  gc.ScreenToCameraMatrix = pViewData->m_InverseProjectionMatrix;
  gc.WorldToCameraMatrix = pViewData->m_ViewMatrix;
  gc.CameraToWorldMatrix = pViewData->m_InverseViewMatrix;
  gc.WorldToScreenMatrix = pViewData->m_ViewProjectionMatrix;
  gc.ScreenToWorldMatrix = pViewData->m_InverseViewProjectionMatrix;
  gc.Viewport = ezVec4(pViewData->m_ViewPortRect.x, pViewData->m_ViewPortRect.y, pViewData->m_ViewPortRect.width, pViewData->m_ViewPortRect.height);

  ezRenderViewContext renderViewContext;
  renderViewContext.m_pCamera = pCamera;
  renderViewContext.m_pViewData = pViewData;
  renderViewContext.m_pRenderContext = pRendererContext;

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    {
      EZ_PROFILE_AND_MARKER(pRendererContext->GetGALContext(), m_Passes[i]->m_ProfilingID);

      m_Passes[i]->Execute(renderViewContext);
    }
  }

  ClearPipelineData(GetPipelineDataForRendering());

  m_CurrentRenderThread = (ezThreadID)0;
}

void ezRenderPipeline::AddPass(ezUniquePtr<ezRenderPipelinePass>&& pPass)
{
  pPass->m_pPipeline = this;
  pPass->InitializePins();

  m_Passes.PushBack(std::move(pPass));
}

void ezRenderPipeline::RemovePass(ezUniquePtr<ezRenderPipelinePass>&& pPass)
{
  m_Passes.Remove(pPass);

  pPass->m_pPipeline = nullptr;
}

ezRenderPipeline::PipelineData* ezRenderPipeline::GetPipelineDataForExtraction()
{
  return &m_Data[ezRenderLoop::GetFrameCounter() & 1];
}

ezRenderPipeline::PipelineData* ezRenderPipeline::GetPipelineDataForRendering()
{
  const ezUInt32 uiFrameCounter = ezRenderLoop::GetFrameCounter() + (CVarMultithreadedRendering ? 1 : 0);
  return &m_Data[uiFrameCounter & 1];
}

const ezRenderPipeline::PipelineData* ezRenderPipeline::GetPipelineDataForRendering() const
{
  const ezUInt32 uiFrameCounter = ezRenderLoop::GetFrameCounter() + (CVarMultithreadedRendering ? 1 : 0);
  return &m_Data[uiFrameCounter & 1];
}


// static
void ezRenderPipeline::ClearPipelineData(PipelineData* pPipeLineData)
{
  for (ezUInt32 uiPassIndex = 0; uiPassIndex < pPipeLineData->m_PassData.GetCount(); ++uiPassIndex)
  {
    PassData& data = pPipeLineData->m_PassData[uiPassIndex];

    data.m_RenderData.Clear();
  }
}

//static 
ezRenderPassType ezRenderPipeline::FindOrRegisterPassType(const char* szPassTypeName)
{
  EZ_ASSERT_RELEASE(MAX_PASS_TYPES > s_uiNextPassType, "Reached the maximum of %d pass types.", MAX_PASS_TYPES);
  
  ezTempHashedString passTypeName(szPassTypeName);

  for (ezRenderPassType type = 0; type < MAX_PASS_TYPES; ++type)
  {
    if (s_PassTypeData[type].m_sName == passTypeName)
      return type;
  }

  ezRenderPassType newType = s_uiNextPassType;
  s_PassTypeData[newType].m_sName.Assign(szPassTypeName);
  s_PassTypeData[newType].m_ProfilingID = ezProfilingSystem::CreateId(szPassTypeName);

  ++s_uiNextPassType;
  return newType;
}

ezRenderPassType ezDefaultPassTypes::Opaque = ezRenderPipeline::FindOrRegisterPassType("Opaque");
ezRenderPassType ezDefaultPassTypes::Masked = ezRenderPipeline::FindOrRegisterPassType("Masked");
ezRenderPassType ezDefaultPassTypes::Transparent = ezRenderPipeline::FindOrRegisterPassType("Transparent");
ezRenderPassType ezDefaultPassTypes::Foreground = ezRenderPipeline::FindOrRegisterPassType("Foreground");



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);

