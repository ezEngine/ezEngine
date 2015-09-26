#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

ezPickingRenderPass::ezPickingRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup) : ezRenderPipelinePass( "SimpleRenderPass" )
{
  m_bEnable = true;
  m_RenderTargetSetup = RenderTargetSetup;
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
}

ezPickingRenderPass::~ezPickingRenderPass()
{

}

void ezPickingRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  if (!m_bEnable)
    return;

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::WireframeColor:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_COLOR");
    break;
  case ezViewRenderMode::WireframeMonochrome:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_MONOCHROME");
    break;
  }

  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  pGALContext->SetRenderTargetSetup(m_RenderTargetSetup);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f));

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "1");

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);

  Event e;
  e.m_Type = Event::Type::AfterOpaque;
  m_Events.Broadcast(e);

  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PICKING", "0");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");

  e.m_Type = Event::Type::EndOfFrame;
  m_Events.Broadcast(e);

  

}
