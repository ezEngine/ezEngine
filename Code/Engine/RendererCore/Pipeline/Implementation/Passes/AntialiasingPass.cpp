#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/AntialiasingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAntialiasingPass, 1, ezRTTIDefaultAllocator<ezAntialiasingPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Post Processing")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAntialiasingPass::ezAntialiasingPass()
  : ezRenderPipelinePass("AntialiasingPass", true)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Antialiasing.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load antialiasing shader!");
  }
}

ezAntialiasingPass::~ezAntialiasingPass() = default;

ezStatus ezAntialiasingPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<ezRenderPipelinePinConnection const> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  // Validate input
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected "));
  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);

  if (inputDesc.m_SampleCount == ezGALMSAASampleCount::TwoSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
  }
  else if (inputDesc.m_SampleCount == ezGALMSAASampleCount::FourSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
  }
  else if (inputDesc.m_SampleCount == ezGALMSAASampleCount::EightSamples)
  {
    m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
  }
  else
  {
    return ezStatus(ezFmt("Input: Invalid MSAA sample count"));
  }

  // Create output
  ezGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = ezGALMSAASampleCount::None;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  // Add passes
  ezGALDevice* pDevice = ref_graph.GetDevice();

  auto pass = ref_graph.AddGraphicsPass("AntialiasingPass");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    if (!pDevice->GetCapabilities().m_bSupportsMultiSampledArrays)
    {
      EZ_ASSERT_DEV(inputDesc.m_uiArraySize == 1, "Stereo rendering is not supported.");
    }
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return EZ_SUCCESS;
}

ezResult ezAntialiasingPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezAntialiasingPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
