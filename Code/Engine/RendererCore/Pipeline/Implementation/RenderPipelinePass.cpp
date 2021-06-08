#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelinePass, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ACCESSOR_PROPERTY("Name", GetName, SetName),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezColorAttribute(ezColorGammaUB(64, 32, 96))
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRenderPipelinePass::ezRenderPipelinePass(const char* szName, bool bIsStereoAware)
  : m_bActive(true)
  , m_bIsStereoAware(bIsStereoAware)
  , m_pPipeline(nullptr)
{
  m_sName.Assign(szName);
}

ezRenderPipelinePass::~ezRenderPipelinePass() = default;

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

void ezRenderPipelinePass::InitRenderPipelinePass(const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) {}

void ezRenderPipelinePass::ExecuteInactive(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs) {}

void ezRenderPipelinePass::ReadBackProperties(ezView* pView) {}

void ezRenderPipelinePass::RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category, ezRenderDataBatch::Filter filter)
{
  EZ_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), ezRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category, filter);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);

    if (const ezRenderData* pRenderData = batch.GetFirstData<ezRenderData>())
    {
      const ezRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const ezRenderer* pRenderer = ezRenderData::GetCategoryRenderer(category, pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);
