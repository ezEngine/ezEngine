#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleRenderPass, 1, ezRTTIDefaultAllocator<ezSimpleRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
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

ezSimpleRenderPass::ezSimpleRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezSimpleRenderPass::~ezSimpleRenderPass() = default;

ezStatus ezSimpleRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTargets& renderTargets = viewData.GetActiveRenderTargets();

  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;

  // If no color input, create from view's render target
  if (hColor.IsInvalidated())
  {
    const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]);
    if (pTexture)
    {
      ezGALTextureCreationDescription desc = pTexture->GetDescription();
      desc.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::ShaderResource);
      desc.m_ResourceAccess.m_bImmutable = true;
      desc.m_pExisitingNativeObject = nullptr;
      hColor = ref_graph.CreateTexture(desc);
    }
  }
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  // If no depth input, create from view's depth target
  if (hDepthStencil.IsInvalidated())
  {
    const ezGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hDSTarget);
    if (pTexture)
    {
      hDepthStencil = ref_graph.CreateTexture(pTexture->GetDescription());
    }
  }
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  if (!hColor.IsInvalidated())
    pass.AddColorTarget(hColor);
  if (!hDepthStencil.IsInvalidated())
    pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    ezTempHashedString sRenderPass("RENDER_PASS_FORWARD");
    if (renderViewContext.m_pViewData->m_ViewRenderMode != ezViewRenderMode::None)
    {
      sRenderPass = ezViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
    }
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleOpaque);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleTransparent);

    if (!m_sMessage.IsEmpty())
    {
      ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), ezVec2I32(20, 20), ezColor::OrangeRed);
    }

    ezDebugRenderer::RenderWorldSpace(renderViewContext);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::GUI);

    ezDebugRenderer::RenderScreenSpace(renderViewContext); });

  return EZ_SUCCESS;
}

ezResult ezSimpleRenderPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sMessage;
  return EZ_SUCCESS;
}

ezResult ezSimpleRenderPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sMessage;
  return EZ_SUCCESS;
}

void ezSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
