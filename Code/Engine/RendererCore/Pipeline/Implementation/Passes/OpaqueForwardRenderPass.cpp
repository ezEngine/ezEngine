#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOpaqueForwardRenderPass, 1, ezRTTIDefaultAllocator<ezOpaqueForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    EZ_MEMBER_PROPERTY("ShadowMasks", m_PinShadowMasks),
    EZ_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezOpaqueForwardRenderPass::ezOpaqueForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezOpaqueForwardRenderPass::~ezOpaqueForwardRenderPass() = default;

ezStatus ezOpaqueForwardRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  EZ_IGNORE_UNUSED(viewData);

  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  EZ_SUCCEED_OR_RETURN(ValidateMatchingTexture(ref_graph, hColor, "Color", hDepthStencil, "DepthStencil", false, true));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  ezRenderGraphTextureHandle hSSAO = inputs[m_PinSSAO.m_uiInputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hShadowMask = inputs[m_PinShadowMasks.m_uiInputIndex].m_TextureHandle;

  EZ_SUCCEED_OR_RETURN(ValidateMatchingTexture(ref_graph, hColor, "Color", hSSAO, "SSAO", true, false));
  EZ_SUCCEED_OR_RETURN(ValidateMatchingTexture(ref_graph, hColor, "Color", hShadowMask, "ShadowMasks", true, false));

  if (!hSSAO.IsInvalidated() && m_ShadingQuality == ezForwardRenderShadingQuality::Simplified)
  {
    ezLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
  }

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  pass.AddDepthStencilTarget(hDepthStencil);
  if (!hSSAO.IsInvalidated())
    pass.ReadTexture(hSSAO, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  if (!hShadowMask.IsInvalidated())
    pass.ReadTexture(hShadowMask, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  DeclareRenderObjectDependencies(ref_graph, pass);
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();
      SetupPermutationVars(renderViewContext);

      // Bind SSAO texture
      ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      if (m_ShadingQuality == ezForwardRenderShadingQuality::Normal)
      {
        if (!hSSAO.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("SSAOTexture", ctx.ResolveTexture(hSSAO));
        }
        else
        {
          bindGroupRenderPass.BindTexture("SSAOTexture", m_hWhiteTexture, ezResourceAcquireMode::BlockTillLoaded);
        }

        if (!hShadowMask.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("ShadowMasksTexture", ctx.ResolveTexture(hShadowMask));
        }
        else
        {
          bindGroupRenderPass.BindTexture("ShadowMasksTexture", m_hWhiteTexture, ezResourceAcquireMode::BlockTillLoaded);
        }
      }
      RenderObjects(renderViewContext); //
    });

  return EZ_SUCCESS;
}

void ezOpaqueForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void ezOpaqueForwardRenderPass::DeclareRenderObjectDependencies(ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_pass)
{
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitOpaqueStatic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitOpaqueDynamic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitMaskedStatic, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitMaskedDynamic, ref_graph, ref_pass);
}

void ezOpaqueForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueStatic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueDynamic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedStatic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedDynamic);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
