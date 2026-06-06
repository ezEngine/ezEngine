#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransparentForwardRenderPass, 1, ezRTTIDefaultAllocator<ezTransparentForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTransparentForwardRenderPass::ezTransparentForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezTransparentForwardRenderPass::~ezTransparentForwardRenderPass()
{
  ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hSceneColorSamplerState);
}

ezStatus ezTransparentForwardRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;
  if (hDepthStencil.IsInvalidated())
    return ezStatus(ezFmt("DepthStencil: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  ezRenderGraphTextureHandle hResolvedDepth = inputs[m_PinResolvedDepth.m_uiInputIndex].m_TextureHandle;

  // Create temp scene color texture
  const ezGALTextureCreationDescription colorDesc = ref_graph.GetTextureDesc(hColor);
  ezGALTextureCreationDescription sceneColorDesc;
  sceneColorDesc.SetAsRenderTarget(colorDesc.m_uiWidth, colorDesc.m_uiHeight, colorDesc.m_Format);
  sceneColorDesc.m_Type = ezGALTextureType::Texture2DArray;
  sceneColorDesc.m_uiArraySize = colorDesc.m_uiArraySize;
  sceneColorDesc.m_uiMipLevelCount = 1;
  ezRenderGraphTextureHandle hSceneColor = ref_graph.CreateTexture(sceneColorDesc);

  // Transparent Pass1
  {
    CreateSamplerState();

    auto pass = ref_graph.AddGraphicsPass("TransparentForward1");
    pass.AddColorTarget(hColor);
    pass.AddDepthStencilTarget(hDepthStencil);
    pass.ReadTexture(hSceneColor, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    if (!hResolvedDepth.IsInvalidated())
      pass.ReadTexture(hResolvedDepth, {}, ezGALResourceState::ShaderResource);
    pass.SetStereoscopic(camera.IsStereoscopic());
    // BEGIN-DOCS-CODE-SNIPPET: renderpass-render-objects
    ezRenderPipelinePass::SetupResourceDependencies(viewData, ref_graph, pass, m_ShadingQuality);
    DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitMeshDecal, ref_graph, pass);
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
        const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
        renderViewContext.UpdateViewport();
        ezRenderPipelinePass::BindDataProviderResources(renderViewContext, m_ShadingQuality);
        ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
        if (!hResolvedDepth.IsInvalidated())
        {
          bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
        }
        RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMeshDecal); //
      });
    // END-DOCS-CODE-SNIPPET
  }

  // Copy current color to scene color texture
  {
    auto transferPass = ref_graph.AddTransferPass("CopySceneColor");
    transferPass.ReadTexture(hColor, {}, ezGALResourceState::ResolveSource);
    transferPass.WriteTexture(hSceneColor, {}, ezGALResourceState::ResolveDestination);
    transferPass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      ezGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hSceneColor), subresource, ctx.ResolveTexture(hColor), subresource); });
  }

  // Transparent pass 2
  {
    auto pass = ref_graph.AddGraphicsPass("TransparentForward2");
    pass.AddColorTarget(hColor);
    pass.AddDepthStencilTarget(hDepthStencil);
    pass.ReadTexture(hSceneColor, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    if (!hResolvedDepth.IsInvalidated())
      pass.ReadTexture(hResolvedDepth, {}, ezGALResourceState::ShaderResource);
    pass.SetStereoscopic(camera.IsStereoscopic());
    ezRenderPipelinePass::SetupResourceDependencies(viewData, ref_graph, pass, m_ShadingQuality);
    DeclareRenderObjectDependencies(ref_graph, pass);
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
      renderViewContext.UpdateViewport();
      SetupPermutationVars(renderViewContext);
      ezRenderPipelinePass::BindDataProviderResources(renderViewContext, m_ShadingQuality);

      ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindTexture("SceneColor", ctx.ResolveTexture(hSceneColor));
      bindGroupRenderPass.BindSampler("SceneColorSampler", m_hSceneColorSamplerState);
      bindGroupRenderPass.BindTexture("SSAOTexture", m_hWhiteTexture, ezResourceAcquireMode::BlockTillLoaded);
      bindGroupRenderPass.BindTexture("ShadowMasksTexture", m_hWhiteTexture, ezResourceAcquireMode::BlockTillLoaded);

      if (!hResolvedDepth.IsInvalidated())
      {
        bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
      }

      RenderObjects(renderViewContext); });
  }

  return EZ_SUCCESS;
}

void ezTransparentForwardRenderPass::DeclareRenderObjectDependencies(ezRenderGraph& ref_graph, ezRenderGraphPassBuilder& ref_pass)
{
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitTransparent, ref_graph, ref_pass);
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LitForeground, ref_graph, ref_pass);
}

void ezTransparentForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
}

void ezTransparentForwardRenderPass::CreateSamplerState()
{
  if (m_hSceneColorSamplerState.IsInvalidated())
  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_MinFilter = ezGALTextureFilterMode::Linear;
    desc.m_MagFilter = ezGALTextureFilterMode::Linear;
    desc.m_MipFilter = ezGALTextureFilterMode::Linear;
    desc.m_AddressU = ezImageAddressMode::Clamp;
    desc.m_AddressV = ezImageAddressMode::Mirror;
    desc.m_AddressW = ezImageAddressMode::Mirror;

    m_hSceneColorSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(desc);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TransparentForwardRenderPass);
