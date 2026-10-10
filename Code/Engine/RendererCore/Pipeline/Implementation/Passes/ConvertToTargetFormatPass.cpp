#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/ConvertToTargetFormatPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConvertToTargetFormatPass, 1, ezRTTIDefaultAllocator<ezConvertToTargetFormatPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
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

ezConvertToTargetFormatPass::ezConvertToTargetFormatPass()
  : ezRenderPipelinePass("ConvertToTargetFormatPass")
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/ConvertToTargetFormat.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load convert to target format shader!");
}

ezConvertToTargetFormatPass::~ezConvertToTargetFormatPass() = default;

ezStatus ezConvertToTargetFormatPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  if (ezGALResourceFormat::IsDepthFormat(inputDesc.m_Format))
    return ezStatus(ezFmt("Input: Must be a color texture"));

  ezGALDevice* pDevice = ref_graph.GetDevice();
  const ezGALTexture* pTarget = pDevice->GetTexture(viewData.GetActiveRenderTargets().m_hRTs[0]);
  if (pTarget == nullptr)
    return ezStatus(ezFmt("View does not have a valid color target"));

  const ezEnum<ezGALResourceFormat> targetFormat = pTarget->GetDescription().m_Format;
  if (ezGALResourceFormat::IsDepthFormat(targetFormat))
    return ezStatus(ezFmt("The color target of the view has the depth format {}", ezArgEnum(targetFormat)));

  if (inputDesc.m_SampleCount != ezGALMSAASampleCount::None && inputDesc.m_uiArraySize > 1 && !pDevice->GetCapabilities().m_bSupportsMultiSampledArrays)
    return ezStatus(ezFmt("Input: MSAA texture arrays are not supported on this device"));

  ezGALTextureCreationDescription outputDesc;
  outputDesc.SetAsRenderTarget(inputDesc.m_uiWidth, inputDesc.m_uiHeight, inputDesc.m_uiArraySize, targetFormat, inputDesc.m_SampleCount);
  outputDesc.m_Type = inputDesc.m_Type;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("ConvertToTargetFormat");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ConvertToTargetFormatPass);
