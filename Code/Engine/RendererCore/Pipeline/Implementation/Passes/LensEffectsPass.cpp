#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/LensEffectsPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLensEffectsPass, 1, ezRTTIDefaultAllocator<ezLensEffectsPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
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

ezLensEffectsPass::ezLensEffectsPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezLensEffectsPass::~ezLensEffectsPass() = default;

ezStatus ezLensEffectsPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  // Pass-through color
  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  ezRenderGraphTextureHandle hResolvedDepth = inputs[m_PinResolvedDepth.m_uiInputIndex].m_TextureHandle;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  if (!hResolvedDepth.IsInvalidated())
    pass.ReadTexture(hResolvedDepth, {}, ezGALResourceState::ShaderResource);
  pass.SetStereoscopic(camera.IsStereoscopic());
  SetupResourceDependencies(viewData, ref_graph, pass);
  DeclareRendererDependenciesForCategory(ezDefaultRenderDataCategories::LensEffects, ref_graph, pass);
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    BindDataProviderResources(renderViewContext);
    //Needed? SetupPermutationVars(renderViewContext);
    if (!hResolvedDepth.IsInvalidated())
    {
      ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
      bindGroupRenderPass.BindTexture("SceneDepth", ctx.ResolveTexture(hResolvedDepth));
    }
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LensEffects); });

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LensEffectsPass);
