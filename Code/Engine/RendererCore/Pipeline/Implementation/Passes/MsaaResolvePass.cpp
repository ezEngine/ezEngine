#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsaaResolvePass, 2, ezRTTIDefaultAllocator<ezMsaaResolvePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("GuaranteeCopy", m_bGuaranteeCopy)->AddAttributes(new ezDefaultValueAttribute(true)),
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

ezMsaaResolvePass::ezMsaaResolvePass()
  : ezRenderPipelinePass("MsaaResolvePass", true)

{
  {
    // Load shader.
    m_hDepthResolveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/MsaaDepthResolve.ezShader");
    EZ_ASSERT_DEV(m_hDepthResolveShader.IsValid(), "Could not load depth resolve shader!");
  }
}

ezMsaaResolvePass::~ezMsaaResolvePass() = default;

ezStatus ezMsaaResolvePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  m_bIsDepth = ezGALResourceFormat::IsDepthFormat(inputDesc.m_Format);
  m_MsaaSampleCount = inputDesc.m_SampleCount;

  const bool bCopy = (m_MsaaSampleCount == ezGALMSAASampleCount::None);

  if (bCopy && !m_bGuaranteeCopy)
  {
    outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hInput;
    return EZ_SUCCESS;
  }

  ezGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = ezGALMSAASampleCount::None;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  if (bCopy)
  {
    auto pass = ref_graph.AddTransferPass(m_bIsDepth ? "CopyNonMsaaDepth" : "CopyNonMsaaColor");
    pass.ReadTexture(hInput, {}, ezGALResourceState::CopySource);
    pass.WriteTexture(hOutput, {}, ezGALResourceState::CopyDestination);
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      { ctx.GetCommandEncoder()->CopyTexture(ctx.ResolveTexture(hOutput), ctx.ResolveTexture(hInput)); });

    return EZ_SUCCESS;
  }

  if (!ref_graph.GetDevice()->GetCapabilities().m_bSupportsMultiSampledArrays)
  {
    EZ_ASSERT_DEV(inputDesc.m_uiArraySize == 1, "Stereo rendering is not supported.");
  }

  if (m_bIsDepth)
  {
    auto pass = ref_graph.AddGraphicsPass("MsaaDepthResolve");
    pass.AddDepthStencilTarget(hOutput);
    pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
    pass.SetStereoscopic(camera.IsStereoscopic());
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

      auto& globals = renderViewContext.m_pRenderContext->WriteGlobalConstants();
      globals.NumMsaaSamples = m_MsaaSampleCount;

      renderViewContext.m_pRenderContext->BindShader(m_hDepthResolveShader);
      renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
      bindGroup.BindTexture("DepthTexture", ctx.ResolveTexture(hInput));

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });
  }
  else
  {
    bool bStereo = camera.IsStereoscopic();
    auto pass = ref_graph.AddTransferPass("MsaaColorResolve");
    pass.ReadTexture(hInput, {}, ezGALResourceState::ResolveSource);
    pass.WriteTexture(hOutput, {}, ezGALResourceState::ResolveDestination);
    pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
      {
      ezGALTextureSubresource subresource;
      subresource.m_uiMipLevel = 0;
      subresource.m_uiArraySlice = 0;
      ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hOutput), subresource, ctx.ResolveTexture(hInput), subresource);

      if (bStereo)
      {
        subresource.m_uiArraySlice = 1;
        ctx.GetCommandEncoder()->ResolveTexture(ctx.ResolveTexture(hOutput), subresource, ctx.ResolveTexture(hInput), subresource);
      } });
  }

  return EZ_SUCCESS;
}

ezResult ezMsaaResolvePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_bGuaranteeCopy;
  return EZ_SUCCESS;
}

ezResult ezMsaaResolvePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  if (uiVersion >= 2)
  {
    inout_stream >> m_bGuaranteeCopy;
  }

  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
