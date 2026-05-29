#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsaaResolvePass, 1, ezRTTIDefaultAllocator<ezMsaaResolvePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
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
  if (inputDesc.m_SampleCount == ezGALMSAASampleCount::None)
    return ezStatus(ezFmt("Input is not a valid msaa target"));

  m_bIsDepth = ezGALResourceFormat::IsDepthFormat(inputDesc.m_Format);
  m_MsaaSampleCount = inputDesc.m_SampleCount;

  ezGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = ezGALMSAASampleCount::None;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

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
      renderViewContext.UpdateViewport();

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



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaResolvePass);
