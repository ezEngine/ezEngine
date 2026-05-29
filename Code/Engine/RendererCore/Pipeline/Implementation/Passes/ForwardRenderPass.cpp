#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderGraph/RenderGraphPassBuilder.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezForwardRenderPass, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_ENUM_MEMBER_PROPERTY("ShadingQuality", ezForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new ezDefaultValueAttribute((int)ezForwardRenderShadingQuality::Normal)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezForwardRenderPass::ezForwardRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
  , m_ShadingQuality(ezForwardRenderShadingQuality::Normal)
{
  m_hWhiteTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
}

ezForwardRenderPass::~ezForwardRenderPass() = default;

ezStatus ezForwardRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepthStencil.IsInvalidated())
    return ezStatus(ezFmt("DepthStencil: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());
  ezRenderPipelinePass::SetupResourceDependencies(viewData, ref_graph, pass, m_ShadingQuality);
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();
    SetupPermutationVars(renderViewContext);
    ezRenderPipelinePass::BindDataProviderResources(renderViewContext, m_ShadingQuality);
    RenderObjects(renderViewContext); });

  return EZ_SUCCESS;
}

ezResult ezForwardRenderPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_ShadingQuality;
  return EZ_SUCCESS;
}

ezResult ezForwardRenderPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_ShadingQuality;
  return EZ_SUCCESS;
}

void ezForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
  ezTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != ezViewRenderMode::None)
  {
    sRenderPass = ezViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  ezStringBuilder sDebugText;
  ezViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, ezVec2I32(10, 10), ezColor::White);
  }

  // Set permutation for shading quality
  if (m_ShadingQuality == ezForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == ezForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    EZ_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
