#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/UpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/UpscaleConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUpscalePass, 2, ezRTTIDefaultAllocator<ezUpscalePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("Sharpness", m_fSharpness)->AddAttributes(new ezDefaultValueAttribute(0.5f), new ezClampValueAttribute(0.0f, 1.0f)),
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

ezUpscalePass::ezUpscalePass()
  : ezRenderPipelinePass("UpscalePass", true)
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Upscale.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load upscale shader!");

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezUpscaleConstants>();
}

ezUpscalePass::~ezUpscalePass()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

ezStatus ezUpscalePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  if (inputDesc.m_SampleCount != ezGALMSAASampleCount::None)
    return ezStatus(ezFmt("Input: Must be resolved (non-MSAA)"));

  const ezUInt32 uiWidth = static_cast<ezUInt32>(viewData.m_ViewPortRect.width);
  const ezUInt32 uiHeight = static_cast<ezUInt32>(viewData.m_ViewPortRect.height);

  if (inputDesc.m_uiWidth == uiWidth && inputDesc.m_uiHeight == uiHeight)
  {
    outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hInput;
    return EZ_SUCCESS;
  }

  ezGALTextureCreationDescription outputDesc;
  outputDesc.SetAsRenderTarget(uiWidth, uiHeight, inputDesc.m_uiArraySize, inputDesc.m_Format);
  outputDesc.m_Type = inputDesc.m_Type;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("Upscale");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    ezUpscaleConstants* pConstants = ezRenderContext::GetConstantBufferData<ezUpscaleConstants>(m_hConstantBuffer);
    pConstants->InputTexelSize = ezVec2(1.0f / inputDesc.m_uiWidth, 1.0f / inputDesc.m_uiHeight);

    // Bilinear upscaling blurs more the smaller the input is, so the sharpening ramps up from nothing at full resolution and reaches
    // m_fSharpness at half the resolution. Without this, a scale just below 100% would be noticeably sharper than 100% itself.
    const float fScale = static_cast<float>(inputDesc.m_uiWidth) / static_cast<float>(uiWidth);
    pConstants->Sharpness = m_fSharpness * ezMath::Saturate((1.0f - fScale) / 0.5f);

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindBuffer("ezUpscaleConstants", m_hConstantBuffer);
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return EZ_SUCCESS;
}

ezResult ezUpscalePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fSharpness;
  return EZ_SUCCESS;
}

ezResult ezUpscalePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  if (uiVersion >= 2)
  {
    inout_stream >> m_fSharpness;
  }
  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_UpscalePass);
