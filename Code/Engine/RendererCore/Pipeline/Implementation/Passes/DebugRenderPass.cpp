#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/Passes/DebugRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDebugWorldRenderPass, 1, ezRTTIDefaultAllocator<ezDebugWorldRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDebugScreenRenderPass, 1, ezRTTIDefaultAllocator<ezDebugScreenRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
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

//////////////////////////////////////////////////////////////////////////

ezDebugWorldRenderPass::ezDebugWorldRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezDebugWorldRenderPass::~ezDebugWorldRenderPass() = default;

ezStatus ezDebugWorldRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hDepthStencil = inputs[m_PinDepthStencil.m_uiInputIndex].m_TextureHandle;

  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;
  outputs[m_PinDepthStencil.m_uiOutputIndex].m_TextureHandle = hDepthStencil;

  // A depth buffer of a different size (e.g. before an ezUpscalePass) can't be bound together with the color target.
  if (!hDepthStencil.IsInvalidated())
  {
    const ezGALTextureCreationDescription& colorDesc = ref_graph.GetTextureDesc(hColor);
    const ezGALTextureCreationDescription& depthDesc = ref_graph.GetTextureDesc(hDepthStencil);
    if (colorDesc.m_uiWidth != depthDesc.m_uiWidth || colorDesc.m_uiHeight != depthDesc.m_uiHeight)
    {
      hDepthStencil.Invalidate();
    }
    else if (colorDesc.m_SampleCount != depthDesc.m_SampleCount)
    {
      return ezStatus(ezFmt("DepthStencil: MSAA mode ({}) doesn't match the one of Color ({}). Connect a depth buffer with the same MSAA mode, e.g. the output of an ezMsaaResolvePass.", ezArgEnum(depthDesc.m_SampleCount), ezArgEnum(colorDesc.m_SampleCount)));
    }
  }

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  if (!hDepthStencil.IsInvalidated())
    pass.AddDepthStencilTarget(hDepthStencil);
  pass.SetStereoscopic(camera.IsStereoscopic());

  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    ezDebugRenderer::RenderWorldSpace(renderViewContext); });

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezDebugScreenRenderPass::ezDebugScreenRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezDebugScreenRenderPass::~ezDebugScreenRenderPass() = default;

ezStatus ezDebugScreenRenderPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hColor = inputs[m_PinColor.m_uiInputIndex].m_TextureHandle;
  if (hColor.IsInvalidated())
    return ezStatus(ezFmt("Color: Not connected"));

  outputs[m_PinColor.m_uiOutputIndex].m_TextureHandle = hColor;

  auto pass = ref_graph.AddGraphicsPass(GetName());
  pass.AddColorTarget(hColor);
  pass.SetStereoscopic(camera.IsStereoscopic());

  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    if (!m_sMessage.IsEmpty())
    {
      ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage.GetData(), ezVec2I32(20, 20), ezColor::OrangeRed);
    }

    ezDebugRenderer::RenderScreenSpace(renderViewContext); });

  return EZ_SUCCESS;
}

ezResult ezDebugScreenRenderPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sMessage;
  return EZ_SUCCESS;
}

ezResult ezDebugScreenRenderPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  inout_stream >> m_sMessage;
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DebugRenderPass);
