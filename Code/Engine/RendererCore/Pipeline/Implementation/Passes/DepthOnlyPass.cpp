#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Foundation/IO/TypeVersionContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDepthOnlyPass, 3, ezRTTIDefaultAllocator<ezDepthOnlyPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("RenderStaticObjects", m_bRenderStaticObjects)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("RenderDynamicObjects", m_bRenderDynamicObjects)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("RenderTransparentObjects", m_bRenderTransparentObjects),
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

ezDepthOnlyPass::ezDepthOnlyPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezDepthOnlyPass::~ezDepthOnlyPass() = default;

ezStatus ezDepthOnlyPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepthStencil.IsInvalidated())
    return ezStatus(ezFmt("DepthStencil: Not connected"));

  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

    if (m_bRenderStaticObjects)
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueStatic);
    if (m_bRenderDynamicObjects)
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueDynamic);
    if (m_bRenderStaticObjects)
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedStatic);
    if (m_bRenderDynamicObjects)
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedDynamic);
    if (m_bRenderTransparentObjects)
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent); });

  return EZ_SUCCESS;
}

// BEGIN-DOCS-CODE-SNIPPET: renderpass-serialization
ezResult ezDepthOnlyPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_bRenderStaticObjects;
  inout_stream << m_bRenderDynamicObjects;
  inout_stream << m_bRenderTransparentObjects;
  return EZ_SUCCESS;
}

ezResult ezDepthOnlyPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  if (uiVersion >= 3)
  {
    inout_stream >> m_bRenderStaticObjects;
    inout_stream >> m_bRenderDynamicObjects;
  }

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRenderTransparentObjects;
  }

  return EZ_SUCCESS;
}
// END-DOCS-CODE-SNIPPET



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
