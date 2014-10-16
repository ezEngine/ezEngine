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

ezRenderPipeline::ezRenderPipeline(ezGALDevice* pDevice, bool bAsynchronous)
{
  m_pDevice = pDevice;
  m_bAsynchronous = bAsynchronous;

  m_pExtractedData = &m_Data[0];
  m_pRenderData = bAsynchronous ? &m_Data[1] : &m_Data[0];
}

ezRenderPipeline::~ezRenderPipeline()
{
  ClearPipelineData(m_pExtractedData);
  ClearPipelineData(m_pRenderData);
}

void ezRenderPipeline::ExtractData(ezWorld& world, const ezCamera& camera)
{
  // swap data
  if (m_bAsynchronous)
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
    ezGameObject* pObject = it;
    pObject->SendMessage(msg);
  }

  for (ezUInt32 uiPassIndex = 0; uiPassIndex < m_pExtractedData->m_PassData.GetCount(); ++uiPassIndex)
  {
    PassData& data = m_pExtractedData->m_PassData[uiPassIndex];
    data.SortRenderData();
  }
}

void ezRenderPipeline::Render(const ezCamera& camera)
{
  // swap data
  if (m_bAsynchronous)
  {
    m_pRenderData = (m_pRenderData == &m_Data[0]) ? &m_Data[1] : &m_Data[0];
  }

  for (ezUInt32 i = 0; i < m_Passes.GetCount(); ++i)
  {
    m_Passes[i]->Run(camera);
  }
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
  EZ_ASSERT(MAX_PASS_TYPES > s_uiNextPassType, "Reached the maximum of %d pass types.", MAX_PASS_TYPES);

  ezRenderPassType newType = s_uiNextPassType;
  s_PassTypeData[newType].m_sName.Assign(szPassTypeName);
  s_PassTypeData[newType].m_ProfilingID = ezProfilingSystem::CreateId(szPassTypeName);

  ++s_uiNextPassType;
  return newType;
}

ezRenderPassType ezDefaultPassTypes::Opaque = ezRenderPipeline::RegisterPassType("Opaque");
ezRenderPassType ezDefaultPassTypes::Masked = ezRenderPipeline::RegisterPassType("Masked");
ezRenderPassType ezDefaultPassTypes::Transparent = ezRenderPipeline::RegisterPassType("Transparent");
