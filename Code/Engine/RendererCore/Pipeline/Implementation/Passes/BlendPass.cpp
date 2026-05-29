#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/BlendPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlendConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlendPass, 1, ezRTTIDefaultAllocator<ezBlendPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputA", m_PinInputA),
    EZ_MEMBER_PROPERTY("InputB", m_PinInputB),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("BlendFactor", m_fBlendFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBlendPass::ezBlendPass()
  : ezRenderPipelinePass("BlendPass")
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Blend.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blend shader!");
}

ezBlendPass::~ezBlendPass() = default;

ezStatus ezBlendPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInputA = inputs[m_PinInputA.m_uiInputIndex].m_TextureHandle;
  ezRenderGraphTextureHandle hInputB = inputs[m_PinInputB.m_uiInputIndex].m_TextureHandle;
  if (hInputA.IsInvalidated() || hInputB.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDescA = ref_graph.GetTextureDesc(hInputA);
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(inputDescA);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("Blend");
  pass.AddColorTarget(hOutput, {}, ezGALRenderTargetLoadOp::Clear);
  pass.SetClearColor(0, ezColor(1.0f, 0.0f, 0.0f));
  pass.ReadTexture(hInputA, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.ReadTexture(hInputB, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    ezBlendConstants cb = {};
    cb.BlendFactor = m_fBlendFactor;
    renderViewContext.m_pRenderContext->SetPushConstants("ezBlendConstants", cb);

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("InputA", ctx.ResolveTexture(hInputA));
    bindGroup.BindTexture("InputB", ctx.ResolveTexture(hInputB));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return EZ_SUCCESS;
}

ezResult ezBlendPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fBlendFactor;
  return EZ_SUCCESS;
}

ezResult ezBlendPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fBlendFactor;
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlendPass);
