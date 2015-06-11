#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EditorEngineProcess/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

ezPickingRenderPass::ezPickingRenderPass(ezGALRenderTargetConfigHandle hRTConfig) : ezRenderPipelinePass("SimpleRenderPass")
{
  m_hRTConfig = hRTConfig;
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
}

ezPickingRenderPass::~ezPickingRenderPass()
{

}

void ezPickingRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  pGALContext->SetRenderTargetConfig(m_hRTConfig);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f));

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "1");

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);

  /// \todo this breaks picking positions, but we don't need that atm anyway
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "0");

  Event e;
  m_Events.Broadcast(e);

}
