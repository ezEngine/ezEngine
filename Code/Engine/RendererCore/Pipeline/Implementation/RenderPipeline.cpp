#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/World/World.h>

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
}

ezRenderPipeline::~ezRenderPipeline()
{
  ClearPipelineData(m_pExtractedData);
  ClearPipelineData(m_pRenderData);
}

void ezRenderPipeline::ExtractData(const ezView& view)
{
  // swap data
  if (m_Mode == Asynchronous)
  {
    m_pExtractedData = (m_pExtractedData == &m_Data[0]) ? &m_Data[1] : &m_Data[0];
  }

  ezExtractRenderDataMessage msg;
  msg.m_pRenderPipeline = this;
  msg.m_pView = &view;

  ClearPipelineData(m_pExtractedData);

  /// \todo use spatial data to do visibility culling etc.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
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

void ezRenderPipeline::Render(const ezView& view, ezGALContext* pGALContext)
{
  // swap data
  if (m_Mode == Asynchronous)
  {
    m_pRenderData = (m_pRenderData == &m_Data[0]) ? &m_Data[1] : &m_Data[0];
  }

  // calculate camera matrices
  const ezRectFloat& viewPortRect = view.GetViewport();
  pGALContext->SetViewport(viewPortRect.x, viewPortRect.y, viewPortRect.width, viewPortRect.height, 0.0f, 1.0f);

  ezRenderContext renderContext;
  renderContext.m_pView = &view;
  renderContext.m_pGALContext = pGALContext;

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    m_Passes[i]->Run(renderContext);
  }
}

void ezRenderPipeline::AddPass(ezRenderPipelinePass* pPass)
{
  pPass->m_pPipeline = this;

  m_Passes.PushBack(pPass);
}

void ezRenderPipeline::RemovePass(ezRenderPipelinePass* pPass)
{
  m_Passes.Remove(pPass);

  pPass->m_pPipeline = nullptr;
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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipeline);

