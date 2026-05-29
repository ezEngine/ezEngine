#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/StereoTestPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Core/Graphics/Camera.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStereoTestPass, 1, ezRTTIDefaultAllocator<ezStereoTestPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Unit Tests")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStereoTestPass::ezStereoTestPass()
  : ezRenderPipelinePass("StereoTestPass", true)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/StereoTest.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load stereo test shader!");
  }
}

ezStereoTestPass::~ezStereoTestPass() = default;

ezStatus ezStereoTestPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  ezGALTextureCreationDescription outputDesc = inputDesc;
  outputDesc.m_SampleCount = ezGALMSAASampleCount::None;
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(outputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddGraphicsPass("StereoTest");
  pass.AddColorTarget(hOutput);
  pass.ReadTexture(hInput, {}, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::PixelShader);
  pass.SetStereoscopic(camera.IsStereoscopic());
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    {
    const ezRenderViewContext& renderViewContext = *ctx.GetUserData<ezRenderViewContext>();
    renderViewContext.UpdateViewport();

    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

    ezBindGroupBuilder& bindGroup = renderViewContext.m_pRenderContext->GetBindGroup();
    bindGroup.BindTexture("ColorTexture", ctx.ResolveTexture(hInput));

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult(); });

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_StereoTestPass);
