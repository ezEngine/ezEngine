#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RendererRegistry.h>
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
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Grape))
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezForwardRenderShadingQuality, 1)
  EZ_ENUM_CONSTANTS(ezForwardRenderShadingQuality::Normal, ezForwardRenderShadingQuality::Simplified)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezRenderPipelinePass::ezRenderPipelinePass(const char* szName, bool bIsStereoAware)
  : m_bIsStereoAware(bIsStereoAware)

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

void ezRenderPipelinePass::ReadBackProperties(ezView* pView) {}

ezResult ezRenderPipelinePass::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return EZ_SUCCESS;
}

ezResult ezRenderPipelinePass::Deserialize(ezStreamReader& inout_stream)
{
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return EZ_SUCCESS;
}

void ezRenderPipelinePass::RenderDataWithCategory(const ezRenderViewContext& renderViewContext, ezRenderData::Category category)
{
  EZ_PROFILE_AND_MARKER(renderViewContext.m_pRenderContext->GetCommandEncoder(), ezRenderData::GetCategoryName(category));

  auto batchList = m_pPipeline->GetRenderDataBatchesWithCategory(category);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);

    if (const ezRenderData* pRenderData = batch.GetFirstData<ezRenderData>())
    {
      const ezRTTI* pType = pRenderData->GetDynamicRTTI();

      if (const ezRenderer* pRenderer = ezRendererRegistry::GetRenderer(pType))
      {
        pRenderer->RenderBatch(renderViewContext, this, batch);
      }
    }
  }
}

void ezRenderPipelinePass::BindDataProviderResources(const ezRenderViewContext& renderViewContext, ezForwardRenderShadingQuality::Enum quality)
{
  // Setup clustered data
  if (quality == ezForwardRenderShadingQuality::Normal)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  // Or other light properties.
  else
  {
    auto pSimplifiedData = GetPipeline()->GetFrameDataProvider<ezSimplifiedDataProvider>()->GetData(renderViewContext);
    pSimplifiedData->BindResources(renderViewContext.m_pRenderContext);
  }
}

void ezRenderPipelinePass::SetupResourceDependencies(const ezViewData& viewData, ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_pass, ezForwardRenderShadingQuality::Enum quality)
{
  if (quality == ezForwardRenderShadingQuality::Normal)
  {
    ezClusteredDataGPU::AddReadDependencies(ref_graph, ref_pass, viewData.m_uiSkyIrradianceIndex, viewData.m_CameraUsageHint);
  }
  else
  {
    ezSimplifiedDataGPU::AddReadDependencies(ref_graph, ref_pass, viewData.m_uiSkyIrradianceIndex, viewData.m_CameraUsageHint);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);
