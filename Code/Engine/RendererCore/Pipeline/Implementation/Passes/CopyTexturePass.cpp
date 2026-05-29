#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/CopyTexturePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: renderpass-reflection
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCopyTexturePass, 1, ezRTTIDefaultAllocator<ezCopyTexturePass>)
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
// END-DOCS-CODE-SNIPPET
// clang-format on

ezCopyTexturePass::ezCopyTexturePass()
  : ezRenderPipelinePass("CopyTexturePass")
{
}

ezCopyTexturePass::~ezCopyTexturePass() = default;

// BEGIN-DOCS-CODE-SNIPPET: renderpass-add-render-passes
ezStatus ezCopyTexturePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  ezRenderGraphTextureHandle hInput = inputs[m_PinInput.m_uiInputIndex].m_TextureHandle;
  if (hInput.IsInvalidated())
    return ezStatus(ezFmt("Input: Not connected"));

  const ezGALTextureCreationDescription inputDesc = ref_graph.GetTextureDesc(hInput);
  ezRenderGraphTextureHandle hOutput = ref_graph.CreateTexture(inputDesc);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hOutput;

  auto pass = ref_graph.AddTransferPass("CopyTexture");
  pass.ReadTexture(hInput, {}, ezGALResourceState::CopySource);
  pass.WriteTexture(hOutput, {}, ezGALResourceState::CopyDestination);
  pass.SetExecuteCallback([=](const ezRenderGraphContext& ctx)
    { ctx.GetCommandEncoder()->CopyTexture(ctx.ResolveTexture(hOutput), ctx.ResolveTexture(hInput)); });

  return EZ_SUCCESS;
}
// END-DOCS-CODE-SNIPPET



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
