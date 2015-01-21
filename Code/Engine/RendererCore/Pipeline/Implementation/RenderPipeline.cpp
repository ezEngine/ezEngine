#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/World/World.h>
#include <CoreUtils/Graphics/Camera.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderData, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderer, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_IMPLEMENT_MESSAGE_TYPE(ezExtractRenderDataMessage);

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

ezRenderPipeline::ezRenderPipeline(Mode mode)
{
  m_Mode = mode;

  m_pExtractedData = &m_Data[0];
  m_pRenderData = m_Mode == Asynchronous ? &m_Data[1] : &m_Data[0];

  m_ViewPortRect = ezRectFloat(0.0f, 0.0f);
  m_pCurrentCamera = nullptr;
  m_pCurrentContext = nullptr;  
}

ezRenderPipeline::~ezRenderPipeline()
{
  ClearPipelineData(m_pExtractedData);
  ClearPipelineData(m_pRenderData);
}

void ezRenderPipeline::ExtractData(const ezWorld& world, const ezCamera& camera)
{
  // swap data
  if (m_Mode == Asynchronous)
  {
    m_pExtractedData = (m_pExtractedData == &m_Data[0]) ? &m_Data[1] : &m_Data[0];
  }

  ezExtractRenderDataMessage msg;
  msg.m_pRenderPipeline = this;
  msg.m_pCamera = &camera;

  ClearPipelineData(m_pExtractedData);

  /// \todo use spatial data to do visibility culling etc.
  for (auto it = world.GetObjects(); it.IsValid(); ++it)
  {
    const ezGameObject* pObject = it;
    pObject->SendMessage(msg);
  }

  for (ezUInt32 uiPassIndex = 0; uiPassIndex < m_pExtractedData->m_PassData.GetCount(); ++uiPassIndex)
  {
    PassData& data = m_pExtractedData->m_PassData[uiPassIndex];
    data.SortRenderData();
  }
}

void ezRenderPipeline::Render(const ezCamera& camera, ezGALContext* pContext)
{
  // swap data
  if (m_Mode == Asynchronous)
  {
    m_pRenderData = (m_pRenderData == &m_Data[0]) ? &m_Data[1] : &m_Data[0];
  }

  // calculate camera matrices
  camera.GetViewMatrix(m_ViewMatrix);
  /// \todo get depth range from device?
  camera.GetProjectionMatrix(m_ViewPortRect.width / m_ViewPortRect.height, ezProjectionDepthRange::ZeroToOne, m_ProjectionMatrix);
  m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

  pContext->SetViewport(m_ViewPortRect.x, m_ViewPortRect.y, m_ViewPortRect.width, m_ViewPortRect.height, 0.0f, 1.0f);

  m_pCurrentCamera = &camera;
  m_pCurrentContext = pContext;

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    m_Passes[i]->Run();
  }

  m_pCurrentCamera = nullptr;
  m_pCurrentContext = nullptr;
}

void ezRenderPipeline::AddPass(ezRenderPipelinePass* pPass)
{
  pPass->m_pPipeline = this;

  m_Passes.PushBack(pPass);
}

void ezRenderPipeline::RemovePass(ezRenderPipelinePass* pPass)
{

}

// static
void ezRenderPipeline::ClearPipelineData(PipelineData* pPipeLineData)
{
  for (ezUInt32 uiPassIndex = 0; uiPassIndex < pPipeLineData->m_PassData.GetCount(); ++uiPassIndex)
  {
    PassData& data = pPipeLineData->m_PassData[uiPassIndex];

    /// \todo not needed once we use a proper allocator
    for (ezUInt32 i = 0; i < data.m_RenderData.GetCount(); ++i)
    {
      ezRenderData* pRenderData = const_cast<ezRenderData*>(data.m_RenderData[i]);
      EZ_DEFAULT_DELETE(pRenderData);
    }

    data.m_RenderData.Clear();
  }
}

//static 
ezRenderPassType ezRenderPipeline::RegisterPassType(const char* szPassTypeName)
{
  EZ_ASSERT_RELEASE(MAX_PASS_TYPES > s_uiNextPassType, "Reached the maximum of %d pass types.", MAX_PASS_TYPES);

  ezRenderPassType newType = s_uiNextPassType;
  s_PassTypeData[newType].m_sName.Assign(szPassTypeName);
  s_PassTypeData[newType].m_ProfilingID = ezProfilingSystem::CreateId(szPassTypeName);

  ++s_uiNextPassType;
  return newType;
}

ezRenderPassType ezDefaultPassTypes::Opaque = ezRenderPipeline::RegisterPassType("Opaque");
ezRenderPassType ezDefaultPassTypes::Masked = ezRenderPipeline::RegisterPassType("Masked");
ezRenderPassType ezDefaultPassTypes::Transparent = ezRenderPipeline::RegisterPassType("Transparent");



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);

