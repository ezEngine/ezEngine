#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelinePass, 1, ezRTTINoAllocator);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_SET_ACCESSOR_PROPERTY("Renderers", GetRenderers, AddRenderer, RemoveRenderer)->AddFlags(ezPropertyFlags::PointerOwner),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezRenderPipelinePass::ezRenderPipelinePass(const char* szName)
{
  m_sName.Assign(szName);
  m_ProfilingID = ezProfilingSystem::CreateId(szName);

  m_pPipeline = nullptr;
}

ezRenderPipelinePass::~ezRenderPipelinePass()
{
  for (ezUInt32 i = 0; i < m_Renderer.GetCount(); ++i)
  {
    auto pAllocator = m_Renderer[i]->GetDynamicRTTI()->GetAllocator();
    EZ_ASSERT_DEBUG(pAllocator->CanAllocate(), "Can't destroy owned pass!");
    pAllocator->Deallocate(m_Renderer[i]);
  }
  m_Renderer.Clear();
}

ezArrayPtr<ezRenderer* const> ezRenderPipelinePass::GetRenderers() const
{
  return m_Renderer.GetArrayPtr();
}

void ezRenderPipelinePass::AddRenderer(ezRenderer* pRenderer)
{
  ezHybridArray<const ezRTTI*, 8> supportedTypes;
  pRenderer->GetSupportedRenderDataTypes(supportedTypes);

  ezUInt32 uiIndex = m_Renderer.GetCount();
  m_Renderer.PushBack(pRenderer);

  for (ezUInt32 i = 0; i < supportedTypes.GetCount(); ++i)
  {
    m_TypeToRendererIndex.Insert(supportedTypes[i], uiIndex);
  }
}

void ezRenderPipelinePass::RemoveRenderer(ezRenderer* pRenderer)
{
  if (!m_Renderer.Contains(pRenderer))
    return;

  m_Renderer.Remove(pRenderer);

  // Rebuild index table
  m_TypeToRendererIndex.Clear();

  for (ezUInt32 i = 0; i < m_Renderer.GetCount(); ++i)
  {
    ezHybridArray<const ezRTTI*, 8> supportedTypes;
    m_Renderer[i]->GetSupportedRenderDataTypes(supportedTypes);

    for (ezUInt32 j = 0; j < supportedTypes.GetCount(); ++j)
    {
      m_TypeToRendererIndex.Insert(supportedTypes[j], i);
    }
  }
}

void ezRenderPipelinePass::SetName(const char* szName)
{
  m_sName.Assign(szName);
}

const char* ezRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void ezRenderPipelinePass::RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category)
{
  EZ_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetGALContext(), ezRenderData::GetCategoryProfilingID(category));

  auto& batches = m_pPipeline->GetRenderDataBatchesWithCategory(category);
  for (auto& batch : batches)
  {
    const ezRenderData* pRenderData = batch.GetData<ezRenderData>(0);
    const ezRTTI* pType = pRenderData->GetDynamicRTTI();
    
    ezUInt32 uiRendererIndex = ezInvalidIndex;
    if (m_TypeToRendererIndex.TryGetValue(pType, uiRendererIndex))
    {
      m_Renderer[uiRendererIndex]->RenderBatch(renderViewContext, this, batch);
    }
    else
    {
      ezLog::Warning("Could not render object of type '%s' in render pass '%s'. No suitable renderer found.", pType->GetTypeName(), m_sName.GetString().GetData());  
    }
  }  
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);

