#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelinePass, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
    EZ_SET_ACCESSOR_PROPERTY("Renderers", GetRenderers, AddRenderer, RemoveRenderer)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezColorAttribute(ezColorGammaUB(64, 32, 96))
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezRenderPipelinePass::ezRenderPipelinePass(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

  m_pPipeline = nullptr;
}

ezRenderPipelinePass::~ezRenderPipelinePass()
{
  for (auto pRenderer : m_Renderer)
  {
    auto pAllocator = pRenderer->GetDynamicRTTI()->GetAllocator();
    EZ_ASSERT_DEBUG(pAllocator->CanAllocate(), "Can't destroy owned pass!");
    pAllocator->Deallocate(pRenderer);
  }
  m_Renderer.Clear();
}

ezArrayPtr<ezRenderer* const> ezRenderPipelinePass::GetRenderers() const
{
  return m_Renderer.GetArrayPtr();
}

ezRenderer* ezRenderPipelinePass::GetRendererByType(const ezRTTI* pType)
{
  for (auto pRenderer : m_Renderer)
  {
    if (pRenderer->IsInstanceOf(pType))
    {
      return pRenderer;
    }
  }

  return nullptr;
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
  if (!ezStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* ezRenderPipelinePass::GetName() const
{
  return m_sName.GetData();
}

void ezRenderPipelinePass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
}

void ezRenderPipelinePass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
}

void ezRenderPipelinePass::ReadBackProperties(ezView* pView)
{
}

void ezRenderPipelinePass::RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category, ezRenderDataBatch::Filter filter)
{
  EZ_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetGALContext(), ezRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category, filter);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);

    const ezRenderData* pRenderData = batch.GetData<ezRenderData>(0);
    const ezRTTI* pType = pRenderData->GetDynamicRTTI();

    ezUInt32 uiRendererIndex = ezInvalidIndex;
    if (m_TypeToRendererIndex.TryGetValue(pType, uiRendererIndex))
    {
      m_Renderer[uiRendererIndex]->RenderBatch(renderViewContext, this, batch);
    }
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);

