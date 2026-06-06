#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SelectionHighlightPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/SelectionHighlightConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSelectionHighlightPass, 1, ezRTTIDefaultAllocator<ezSelectionHighlightPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),

    EZ_MEMBER_PROPERTY("HighlightColor", m_HighlightColor)->AddAttributes(new ezDefaultValueAttribute(ezColorScheme::LightUI(ezColorScheme::Yellow))),
    EZ_MEMBER_PROPERTY("OverlayOpacity", m_fOverlayOpacity)->AddAttributes(new ezDefaultValueAttribute(0.1f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSelectionHighlightPass::ezSelectionHighlightPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
  // Load shader.
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/SelectionHighlight.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load selection highlight shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSelectionHighlightConstants>();
}

ezSelectionHighlightPass::~ezSelectionHighlightPass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

ezStatus ezSelectionHighlightPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  ezRenderGraphTextureHandle hDepth = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepth.IsInvalidated())
    return ezStatus(ezFmt("DepthStencil: Not connected"));

  // Create temp depth texture for selection rendering
  const ezGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColor);
  ezGALTextureCreationDescription depthDesc;
  depthDesc.SetAsRenderTarget(colorDesc.m_uiWidth, colorDesc.m_uiHeight, colorDesc.m_uiArraySize, ezGALResourceFormat::D24S8, colorDesc.m_SampleCount);
  ezRenderGraphTextureHandle hSelectionDepth = ref_graph.CreateTexture(depthDesc);

  // Render selection objects to depth only
  {
    auto pass = ref_graph.AddGraphicsPass("SelectionDepth");
    pass.AddDepthStencilTarget(hSelectionDepth, {}, ezGALRenderTargetLoadOp::Clear, {}, ezGALRenderTargetLoadOp::Clear);
    pass.SetClearDepth();
    pass.SetClearStencil();
    pass.SetStereoscopic(camera.IsStereoscopic());
    DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::Selection, ref_graph, pass);
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection); });
  }

  // Reconstruct selection overlay from depth
  {
    auto pass = ref_graph.AddGraphicsPass("SelectionHighlight");
    pass.AddColorTarget(hColor);
    pass.ReadTexture(hSelectionDepth, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.ReadTexture(hDepth, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();

      auto constants = ezRenderContext::GetConstantBufferData<ezSelectionHighlightConstants>(m_hConstantBuffer);
      constants->HighlightColor = m_HighlightColor;
      constants->OverlayOpacity = m_fOverlayOpacity;

      renderViewContext.m_pRenderContext->BindShader(m_hShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindBuffer("ezSelectionHighlightConstants", m_hConstantBuffer);
      bindGroupRenderPass.BindTexture("SelectionDepthTexture", ctx.ResolveTexture(hSelectionDepth));
      bindGroupRenderPass.BindTexture("SceneDepthTexture", ctx.ResolveTexture(hDepth));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }

  return EZ_SUCCESS;
}

ezResult ezSelectionHighlightPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_HighlightColor;
  inout_stream << m_fOverlayOpacity;
  return EZ_SUCCESS;
}

ezResult ezSelectionHighlightPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_HighlightColor;
  inout_stream >> m_fOverlayOpacity;
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SelectionHighlightPass);
