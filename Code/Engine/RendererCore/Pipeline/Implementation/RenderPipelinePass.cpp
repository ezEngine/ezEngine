#include <RendererCore/RendererCorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/RendererRegistry.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(ezRenderPipelinePass, 2)
{
  EZ_BEGIN_PROPERTIES
  {
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
  inout_stream << m_sName;
  return EZ_SUCCESS;
}

ezResult ezRenderPipelinePass::Deserialize(ezStreamReader& inout_stream)
{
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_ASSERT_DEBUG(uiVersion >= 1 && uiVersion <= 2, "Unknown version encountered");

  if (uiVersion < 2)
  {
    // The 'Active' flag was removed, passes are disabled through switch passes instead.
    bool bActive = true;
    inout_stream >> bActive;
  }

  inout_stream >> m_sName;
  return EZ_SUCCESS;
}

void ezRenderPipelinePass::DeclareRendererDependenciesForCategory(ezRenderData::Category category, ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_passBuilder)
{
  for (const ezTextureDependency& dep : m_pPipeline->GetTextureDependenciesWithCategory(category))
  {
    ref_passBuilder.ReadTexture(ref_graph.ImportTexture(dep.m_hTexture), {}, dep.m_RequiredState, dep.m_Stage);
  }

  for (const ezBufferDependency& dep : m_pPipeline->GetBufferDependenciesWithCategory(category))
  {
    ref_passBuilder.ReadBuffer(ref_graph.ImportBuffer(dep.m_hBuffer), dep.m_RequiredState, dep.m_Stage);
  }
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

void ezRenderPipelinePass::SetReadBackProperty(ezView* pView, ezStringView sPropertyName, const ezVariant& value)
{
  ezStringBuilder sb = GetName();
  sb.Append(".", sPropertyName);

  pView->GetBlackboard()->SetEntryValue(sb, value);
}

ezStatus ezRenderPipelinePass::ValidateMatchingTexture(const ezRenderGraph& graph, ezRenderGraphTextureHandle hReference, ezStringView sReferencePinName, ezRenderGraphTextureHandle hTexture, ezStringView sPinName, ezBitflags<ezTextureValidationFlags> flags)
{
  if (hTexture.IsInvalidated())
  {
    if (flags.IsSet(ezTextureValidationFlags::Optional))
      return EZ_SUCCESS;

    return ezStatus(ezFmt("{}: Not connected", sPinName));
  }

  if (hReference.IsInvalidated())
    return EZ_SUCCESS;

  const ezGALTextureCreationDescription& referenceDesc = graph.GetTextureDesc(hReference);
  const ezGALTextureCreationDescription& textureDesc = graph.GetTextureDesc(hTexture);

  // Typically happens when one texture comes from before an ezUpscalePass and the other from after it.
  if (textureDesc.m_uiWidth != referenceDesc.m_uiWidth || textureDesc.m_uiHeight != referenceDesc.m_uiHeight)
    return ezStatus(ezFmt("{}: Size ({}x{}) doesn't match the one of {} ({}x{}). Both have to be connected on the same side of an ezUpscalePass.", sPinName, textureDesc.m_uiWidth, textureDesc.m_uiHeight, sReferencePinName, referenceDesc.m_uiWidth, referenceDesc.m_uiHeight));

  if (flags.IsSet(ezTextureValidationFlags::CheckMsaa) && textureDesc.m_SampleCount != referenceDesc.m_SampleCount)
    return ezStatus(ezFmt("{}: MSAA mode ({}) doesn't match the one of {} ({}). Connect a texture with the same MSAA mode, e.g. the output of an ezMsaaResolvePass.", sPinName, ezArgEnum(textureDesc.m_SampleCount), sReferencePinName, ezArgEnum(referenceDesc.m_SampleCount)));

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderPipelinePass);
